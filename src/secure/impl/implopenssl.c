/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "../tlsutil.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string.h>

BIO *bio_err = NULL;

#include "ossl-ocsp.c"

ocsp_data_t ocsp_data = { 0 };

/*#define LOG_ALPNS*/

SSL_CTX *ctx;
typedef const unsigned char *cucp;
const unsigned char alpn_h1[] = "http/1.1";
const unsigned char alpn_h2[] = "h2";

unsigned char data[] = "http/1.1";

int comp(cucp a, cucp b, size_t len) {
	size_t i;
	for (i = 0; i < len; i++) {
		if (a[i] != b[i])
			return 0;
	}
	return 1;
}

TLS_AP tls_get_ap(TLS tls) {
	cucp dat;
	uint32_t len;
	SSL_get0_alpn_selected((SSL *)tls, &dat, &len);
	
	/* if dat == null, alpn wasn't performed, so just use HTTP/1.1 */
	if (!dat)
		return TLS_AP_HTTP11;
	
	/*printf("getap='%s' len=%u p=%p ps=%p\n", dat, len, dat, data);*/
	
	switch (len) {
		 case 8:
		 	if (comp(dat, alpn_h1, len))
		 		return TLS_AP_HTTP11;
		 	break;
		 case 2:
		 	if (comp(dat, alpn_h2, len))
		 		return TLS_AP_HTTP2;
	}
	
	printf("alpn invalid! data=%s\n", dat);
	return TLS_AP_INVALID;
}

static int alpn_handle (SSL *ssl, const unsigned char **out, unsigned char *outlen, 
												const unsigned char *in, unsigned int inlen, void *arg) {
	/* */
	if (inlen == 0) {
		puts("invalid alpn data...");
		return SSL_TLSEXT_ERR_ALERT_FATAL;
	}
	
	TLS_AP ap = TLS_AP_INVALID;
	
#ifdef LOG_ALPNS
	unsigned char allalps[512];
	size_t allp = 0;
#endif
	
	unsigned char c[256];
	size_t i, j;
	for (i = 0; i < inlen; /**/) {
		unsigned int len = in[i++];
		if (len + i > inlen || len + i > 254) {
			puts("invalid alpn data...");
			return SSL_TLSEXT_ERR_ALERT_FATAL;
		}
		
		c[len] = 0;
		for (j = 0; j < len; j++)
			c[j] = in[i+j];
		
		if (len == sizeof(alpn_h1) - 1 && comp(alpn_h1, c, len) && ap < TLS_AP_HTTP11) {
			*out = alpn_h1;
			*outlen = len;
			ap = TLS_AP_HTTP11;
		} else if (len == sizeof(alpn_h2) - 1 && comp(alpn_h2, c, len) && ap < TLS_AP_HTTP2){
			*out = alpn_h2;
			*outlen = len;
			ap = TLS_AP_HTTP2;
		}
		
#ifdef LOG_ALPNS
		memcpy(allalps+allp, in+i, len);
		allalps[allp+len] = ' ';
		allp+=len+1;
#endif
		
		i+=len;
	}
	
	
#ifdef LOG_ALPNS
	allalps[allp-1] = 0;
	printf("ap=%i, all='%s'\n", ap, allalps);
#endif
	
	return SSL_TLSEXT_ERR_OK;
}

int tls_setup(secure_config_t *sconfig) {
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
	
	bio_err = BIO_new(BIO_s_file());
	BIO_set_fp(bio_err, stderr, 0);

	const SSL_METHOD *method;

	method =  SSLv23_server_method();

	ctx = SSL_CTX_new(method);
	if (!ctx) {
		perror("Unable to create SSL context");
		ERR_print_errors_fp(stderr);
		return 0;
	}

	SSL_CTX_set_ecdh_auto(ctx, 1);

	/* Set the key and cert */
	if (SSL_CTX_use_certificate_file(ctx, sconfig->cert, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		return 0;
	}
	
	if (sconfig->chain) {
		FILE *file = fopen(sconfig->chain, "r");
		X509 *cert = PEM_read_X509(file, NULL, 0, NULL);
		fclose(file);
		if (!SSL_CTX_add_extra_chain_cert(ctx, cert)) {
			puts("Failed to load certificate chain :(");
			ERR_print_errors_fp(stderr);
			return 0;
		}
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, sconfig->key, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		return 0;
	}

	if (sconfig->chain) {
		BIO *bio = BIO_new(BIO_s_file());
		X509 *cert = X509_new();
		if (!BIO_read_filename(bio, sconfig->chain))
			perror("chain read failure");
		
		PEM_read_bio_X509(bio, &cert, 0, NULL);

		if (!cert) {
			puts("Failed to load chain");
		}

		if (bio)
			BIO_free(bio);
		
		SSL_CTX_add1_chain_cert(ctx, cert);
	}
	
	if (sconfig->min_protocol_version != PROTOCOL_NULL) {
		SSL_CTX_set_min_proto_version(ctx, TLS1_VERSION + sconfig->min_protocol_version);
	}
	
	if (sconfig->cipher_suites && !SSL_CTX_set_ciphersuites(ctx, sconfig->cipher_suites)) {
		perror("[Secure] Failed to set cipher suites");
	}
	
	if (sconfig->cipher_list && !SSL_CTX_set_cipher_list(ctx, sconfig->cipher_list)) {
		perror("[Secure] Failed to set cipher list");
	}
	
	
	/*STACK_OF(SSL_CIPHER) *stack = SSL_CTX_get_ciphers(ctx);
	printf("%p\n", stack);
	
	size_t i;
	for (i = 0; i < sk_SSL_CIPHER_num(stack); i++) {
		const SSL_CIPHER *cipher = sk_SSL_CIPHER_value(stack, i);
		printf("> '%s'\n", SSL_CIPHER_get_name(cipher));
	}*/
	
	
	/* set ALPN */
	SSL_CTX_set_alpn_select_cb(ctx, alpn_handle, NULL);
	
	
	/* OCSP stapling */
	if (sconfig->ocsp_file) {
		ocsp_data.file = sconfig->ocsp_file;
		
		SSL_CTX_set_tlsext_status_cb(ctx, cert_status_cb);
		SSL_CTX_set_tlsext_status_arg(ctx, &ocsp_data);
	}
	
	return 1;
}

void tls_destroy(void) {
	free(ocsp_data.file);
	BIO_free(bio_err);
	SSL_CTX_free(ctx);
	EVP_cleanup();
}

void *tls_setup_client(int client) {
	SSL *ssl = SSL_new(ctx);
	if (!ssl) {
		puts("[OpenSSL] (Client) Failed to create SSL object!");
		return NULL;
	}
	SSL_set_fd(ssl, client);

	int ret = SSL_accept(ssl);
	if (ret <= 0) {
		ERR_print_errors_fp(stderr);
		/*printf("%i\n", SSL_get_error(ssl, ret));*/
		tls_destroy_client(ssl);
		return NULL;
	}
	
	return ssl;
}

void tls_destroy_client(void *ssl) {
	SSL_shutdown((SSL *) ssl);
	SSL_free((SSL *) ssl);
}

int tls_read_client(void *pssl, char *result, size_t length) {
	return SSL_read((SSL *) pssl, result, length);
}

int tls_read_client_complete(void *pssl, char *result, size_t length) {
	int read = 0;
	size_t bytes_read = 0;
	
	do {
		read = SSL_read((SSL *) pssl, result + bytes_read, length - bytes_read);
		read += bytes_read;
		
		if (!read)
			return 0;
	} while (read != length);
	
	return 1;
}

const char *_get_code(SSL *ssl, int i) {
	switch (SSL_get_error(ssl, i)) {
		case SSL_ERROR_NONE: return "SSL_ERROR_NONE";
		case SSL_ERROR_ZERO_RETURN: return "SSL_ERROR_ZERO_RETURN";
		case SSL_ERROR_WANT_READ: return "SSL_ERROR_WANT_READ/SSL_ERROR_WANT_WRITE";
		case SSL_ERROR_WANT_CONNECT: return "SSL_ERROR_WANT_CONNECT/SSL_ERROR_WANT_ACCEPT";
		case SSL_ERROR_WANT_X509_LOOKUP: return "SSL_ERROR_WANT_X509_LOOKUP";
		case SSL_ERROR_SYSCALL: return "SSL_ERROR_SYSCALL";
		case SSL_ERROR_SSL: return "SSL_ERROR_SSL";
		default: return "?(default)";
	}
}

int tls_write_client(void *pssl, const char *data, size_t length) {
	int i = SSL_write((SSL *) pssl, data, length);
	if (i > 0)
		return 1;
	
	printf("[OpenSSL] Failed to write data. Code=%s ssl=%p data=%p len=%zi\n", _get_code((SSL *) pssl, i), pssl, data, length);
	ERR_print_errors_fp(stderr);
	return 0;
}

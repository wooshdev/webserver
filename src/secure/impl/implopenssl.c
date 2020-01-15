/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "../tlsutil.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

SSL_CTX *ctx;

int tls_setup(secure_config_t *sconfig) {
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();

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
	
	/*if (sconfig->chain && SSL_CTX_use_certificate_chain_file(ctx, sconfig->chain) <= 0) {
		FILE *file = fopen(sconfig->chain, "r");
		X509 *cert = PEM_read_X509(file, NULL, 0, NULL);
		fclose(file);
		SSL_CTX_add_extra_chain_cert(ctx, cert);
		
		puts("Failed to load certificate chain :(");
		ERR_print_errors_fp(stderr);
		return 0;
	}*/

	if (SSL_CTX_use_PrivateKey_file(ctx, sconfig->key, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		return 0;
	}

	if (sconfig->chain) {
		BIO *bio = BIO_new(BIO_s_mem());
		X509 *cert = X509_new();
		/*bio = BIO_new_mem_buf(certData, -1);*/
		if (!BIO_read_filename(bio, sconfig->chain))
			perror("chain read failure");
		PEM_read_bio_X509(bio, &cert, 0, NULL);

		if (!cert) {
        	puts("Failed to load chain");
		}

		if (bio) {
			BIO_free(bio);
		}

		SSL_CTX_add1_chain_cert(ctx, cert);
	}

	const char *names[] = { "TLS 1.0", "TLS 1.1", "TLS 1.2", "TLS 1.3", "TLS 1.4" };
	
	if (sconfig->min_protocol_version != PROTOCOL_NULL) {
		printf("minver was set\n");
		SSL_CTX_set_min_proto_version(ctx, TLS1_VERSION + sconfig->min_protocol_version);
	}
	
	printf("[Secure] Using protocol version: %s\n", names[SSL_CTX_get_min_proto_version(ctx) - TLS1_VERSION]);
	
	printf("[Secure] Using protocol max: %zi\n", SSL_CTX_get_max_proto_version(ctx));
	
	printf("ciphers=%s\ncipher_suites=%s\n", sconfig->cipher_list, sconfig->cipher_suites);
	
	if (sconfig->cipher_suites && !SSL_CTX_set_ciphersuites(ctx, sconfig->cipher_suites)) {
		perror("[Secure] Failed to set cipher suites");
	}
	
	printf("%p %p %p\n", sconfig->cipher_suites, sconfig->cipher_list, sconfig->chain );
	
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
	
	return 1;
}

void tls_destroy(void) {
	SSL_CTX_free(ctx);
	EVP_cleanup();
}

void *tls_setup_client(int client) {
	SSL *ssl = SSL_new(ctx);
	if (!ssl) {
		puts("Failed to create SSL");
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
	return 0;
}

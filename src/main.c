/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "server.h"
#include "configuration/config.h"

void init_openssl() {
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
	EVP_cleanup();
}

SSL_CTX *create_context() {
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	method = SSLv23_server_method();

	ctx = SSL_CTX_new(method);
	if (!ctx) {
	perror("Unable to create SSL context");
	ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
	}

	return ctx;
}

void configure_context(SSL_CTX *ctx) {
	SSL_CTX_set_ecdh_auto(ctx, 1);

	/* Set the key and cert */
	if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0 ) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv) {
	if (argv != NULL) {
		config_t config = config_read("test.ini");
		
		printf("chain: %s\n", config_get(config, "chain"));
		
		config_destroy(config);
		return EXIT_SUCCESS;
	}
	
	
	int sock;
	SSL_CTX *ctx;

	init_openssl();
	ctx = create_context();

	configure_context(ctx);

	sock = server_create_socket(4433);

	/* Handle connections */
	while(1) {
		struct sockaddr_in addr;
		uint32_t len = sizeof(addr);
		SSL *ssl;
		const char reply[] = "test\n";

		int client = accept(sock, (struct sockaddr*)&addr, &len);
		if (client < 0) {
			perror("Unable to accept");
			exit(EXIT_FAILURE);
		}

		ssl = SSL_new(ctx);
		SSL_set_fd(ssl, client);

		if (SSL_accept(ssl) <= 0) {
			ERR_print_errors_fp(stderr);
		}
		else {
			SSL_write(ssl, reply, strlen(reply));
		}

		SSL_shutdown(ssl);
		SSL_free(ssl);
		close(client);
	}

	close(sock);
	SSL_CTX_free(ctx);
	cleanup_openssl();
	
	return EXIT_SUCCESS;
}
 

/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>

#include "server.h"
#include "configuration/config.h"
#include "utils/util.h"
#include "utils/fileutil.h"
#include "secure/tlsutil.h"
#include "http/parser.h"
#include "http/common.h"
#include "utils/io.h"

const char *default_server_name = "wss";

int main(int argc, char **argv) {
	if (!http_parser_setup()) {
		puts("Failed to setup HTTP parser!");
		return EXIT_FAILURE;
	}
	
	config_t config = config_read("test.ini");

	if (!config_validate(config)) {
		puts("[Config] Invalid configuration.\nQuitting!");
		return EXIT_FAILURE;
	}
	
	/* set the http/common.h server name field, which should be used as the 'Server' header value.*/
	strcpy(http_header_server_name, config_get_default(config, "server-name", default_server_name));
	http_headers_strict = config_get_bool(config, "headers-strict", 0);

	secure_config_t *sconfig;
	const char *options[] = { "letsencrypt", "manual" };
	switch (strswitch(config_get(config, "tls-mode"), options, sizeof(options)/sizeof(options[0]), CASEFLAG_IGNORE_A)) {
		case 0:
			sconfig = secure_config_letsencrypt();
			break;
		default:
			printf("[Config] Encountered invalid TLS mode! Value='%s'\n", config_get(config, "tls-mode"));
		case 1:
			sconfig = secure_config_manual(config);
			break;
	}
	secure_config_others(config, sconfig);
	
	printf("Cert:\n\tcert=\"%s\"\n\tchain=\"%s\"\n\tkey=\"%s\"\n", sconfig->cert, sconfig->chain, sconfig->key);

	tls_setup(sconfig);
	
	int sock = server_create_socket((uint16_t)strtoul(config_get(config, "port"), NULL, 0));
	
	/* post-init: */
	free(sconfig);
	config_destroy(config);
	
	puts("");
	
	/* Handle connections */
	uint32_t len = sizeof(struct sockaddr_in);
	while(1) {
		struct sockaddr_in addr;

		int client = accept(sock, (struct sockaddr*)&addr, &len);
		if (client < 0) {
			perror("Unable to accept");
			exit(EXIT_FAILURE);
		}
		
		printf("connection.\n");

		TLS tls = tls_setup_client(client);
		puts("> TLS Success");

		if (tls) {
			/* parse method */
			char *method = calloc(HTTP1_LONGEST_METHOD, sizeof(char));
			if (!method || !http_parse_method(tls, method, HTTP1_LONGEST_METHOD) || /* only support 'GET' atm. */ strcmp(method, "GET")) {
				http_handle_error_gracefully(tls, HTTP_ERROR_UNSUPPORTED_METHOD, method, 0);
				goto clean;
			}
			
			/* parse path */
			char path[HTTP_PATH_MAX];
			path[HTTP_PATH_MAX - 1] = 0;
			if (io_read_until(tls, path, ' ', HTTP_PATH_MAX-1) <= 0) {
				http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_PATH, path, 0);
				goto clean;
			}
			
			/* parse version */
			char version[HTTP_VERSION_MAX];
			version[HTTP_VERSION_MAX - 1] = 0;
			if (io_read_until(tls, version, '\r', HTTP_VERSION_MAX-1) <= 0 || strcmp(version, "HTTP/1.1")) {
				printf("invalid version='%s'\n", version);
				http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_VERSION, version, 0);
				goto clean;
			}
			
			puts("> Parse Success");
			const char *reply = "HTTP/1.1 200 OK\r\nServer: wss\r\nConnection: close\r\nStrict-Transport-Security: max-age=31536000; includeSubDomains; preload\r\n\r\nStill working pls wait.";
			tls_write_client(tls, reply, strlen(reply));

			clean:
			free(method);
			tls_destroy_client(tls);
		}

		close(client);
	}

	close(sock);
	tls_destroy();
	
	return EXIT_SUCCESS;
}
 

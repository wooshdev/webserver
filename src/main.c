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
#include <signal.h>

#include "server.h"
#include "configuration/config.h"
#include "utils/util.h"
#include "utils/fileutil.h"
#include "secure/tlsutil.h"
#include "http/parser.h"
#include "http/common.h"
#include "utils/io.h"
#include "handling/handlers.h"

const char *default_server_name = "wss";
static int socket_initialized = 0;
static int sock;

static void catch_signal(int signo, siginfo_t *info, void *context) {
  if (socket_initialized) {
    close(sock);
  }
}

int main(int argc, char **argv) {
  struct sigaction act;
  memset(&act, 0, sizeof(struct sigaction));
	sigemptyset(&act.sa_mask);
  
	act.sa_sigaction = catch_signal;
	act.sa_flags = SA_SIGINFO;
  
  if (-1 == sigaction(SIGINT, &act, NULL)) {
    fputs("Failed to set signal handler!\n", stderr);
    perror("info");
		exit(EXIT_FAILURE);
	}
  
	if (!http_parser_setup()) {
		fputs("Failed to setup HTTP parser!\n", stderr);
		return EXIT_FAILURE;
	}
	
	config_t config = config_read("test.ini");
	if (!config_validate(config)) {
		fputs("[Config] Invalid configuration.\nQuitting!\n", stderr);
		return EXIT_FAILURE;
	}
	
	/** static configuration options: **/
	/* set the http/common.h server name field, which should be used as the 'Server' header value.*/
	strcpy(http_header_server_name, config_get_default(config, "server-name", default_server_name));
	strcpy(http_host, config_get(config, "hostname"));
	
	http_headers_strict = config_get_bool(config, "headers-strict", 0);
	http_host_strict = config_get_bool(config, "hostname-strict", 0);

	/** handler configuration options: **/
	if (!handle_setup(config)) {
		fputs("[Handlers] Failed setup.\n", stderr);
		config_destroy(config);
		return EXIT_FAILURE;
	}
	
	printf("%li\n", _POSIX_C_SOURCE);
	
	/** secure configuration options: **/
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
	
	sock = server_create_socket((uint16_t)strtoul(config_get(config, "port"), NULL, 0));
  socket_initialized = 1;
	
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
			perror("Client acceptance failure");
			exit(EXIT_FAILURE);
		}
		
		printf("connection.\n");

		TLS tls = tls_setup_client(client);
		puts("> TLS Success");

		if (tls) {
			http_request_t req;
			req.method = calloc(HTTP1_LONGEST_METHOD, sizeof(char));
		
			/* parse method */
			if (!req.method || !http_parse_method(tls, req.method, HTTP1_LONGEST_METHOD) || /* only support 'GET' atm. */ strcmp(req.method, "GET")) {
				http_handle_error_gracefully(tls, HTTP_ERROR_UNSUPPORTED_METHOD, req.method, 0);
				goto clean;
			}
			
			/* parse path */
			req.path[HTTP_PATH_MAX - 1] = 0;
			if (io_read_until(tls, req.path, ' ', HTTP_PATH_MAX-1) <= 0) {
				http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_PATH, req.path, 0);
				goto clean;
			}
			
			/* parse version */
			req.version[HTTP_VERSION_MAX - 1] = 0;
			if (io_read_until(tls, req.version, '\r', HTTP_VERSION_MAX-1) <= 0 || strcmp(req.version, "HTTP/1.1")) {
				printf("invalid version='%s'\n", req.version);
				http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_VERSION, req.version, 0);
				goto clean;
			}
      
      /* remove the last '\n' character from the stream */
      char end_character[1];
      tls_read_client(tls, end_character, 1);
      
			req.headers = http_parse_headers(tls);
      /*printf("header_error=%i\n", headers.error);*/
			
			const char *hostv;
			if (http_host_strict && (hostv = http_get_header(req.headers, "host")) && strcmp(http_host, hostv)) {
				http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_HOST, req.version, 0);
				goto clean;
			}
			puts("> Header Parse Success");
			puts("> Parse Success");
			
			/* handle the request. */
			http_response_t response = handle_request(req);
			tls_write_client(tls, response.content, response.size == 0 ? strlen(response.content) : response.size);

			clean:
			http_destroy_headers(req.headers);
			free(req.method);
			tls_destroy_client(tls);
		}

		close(client);
	}

	close(sock);
	tls_destroy();
	
	return EXIT_SUCCESS;
}
 

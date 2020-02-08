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
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "server.h"
#include "configuration/config.h"
#include "utils/util.h"
#include "utils/fileutil.h"
#include "secure/tlsutil.h"
#include "http/parser.h"
#include "http/common.h"
#include "utils/io.h"
#include "handling/handlers.h"
#include "http/http1.h"
#include "http2/core.h"

/* This array is defined by src/http/common.c */
extern const char *http_common_log_status_names[];

/* How should we log requests? */
typedef enum {
	/* Don't log requests. */
	REQUEST_LOG_NONE,
	
	/* As minimal as possible. */
	REQUEST_LOG_MINIMAL,
	
	/* Log a lot of information (useful for debugging) */
	REQUEST_LOG_VERBOSE,
	
	/* (not an actual type) */
	_REQUEST_LOG_END
	
} REQUEST_LOG_TYPE;

const char *log_prefixes[] = { NULL, "minimal", "verbose" };

const char *default_server_name = "wss";
static int socket_initialized = 0;
static int sock;
REQUEST_LOG_TYPE request_log_type = REQUEST_LOG_MINIMAL;

static void catch_signal(int signo, siginfo_t *info, void *context) {
  if (signo == SIGINT && socket_initialized) {
    close(sock);
  }
}

int main(int argc, char **argv) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	sigemptyset(&act.sa_mask);
  
	act.sa_sigaction = catch_signal;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGINT, &act, NULL) == -1 || sigaction(SIGPIPE, &act, NULL) == -1) {
		fputs("Failed to set signal handler!\n", stderr);
		perror("info");
		exit(EXIT_FAILURE);
	}
  
	if (!http2_setup()) {
		fputs("Failed to setup HTTP/2!\n", stderr);
		return EXIT_FAILURE;
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
	
	/** secure configuration options: **/
	secure_config_t *sconfig;
	const char *tls_mode_options[] = { "letsencrypt", "manual" };
	switch (strswitch(config_get(config, "tls-mode"), tls_mode_options, sizeof(tls_mode_options)/sizeof(tls_mode_options[0]), CASEFLAG_IGNORE_A)) {
		case 0:
			sconfig = secure_config_letsencrypt();
			break;
		default:
			printf("[Config] Encountered invalid TLS mode! Value='%s'\n", config_get(config, "tls-mode"));
		case 1:
			sconfig = secure_config_manual(config);
			break;
	}
	
	if (!secure_config_others(config, sconfig)) {
		puts("Secure config failure.");
		free(sconfig);
		config_destroy(config);
		return EXIT_FAILURE;
	}
	
	/* Request log type: */
	const char *request_log_type_s = config_get(config, "log-request");
	if (request_log_type_s) {
		const char *request_log_type_options[] = { "none", "minimal", "verbose" };
		int i = strswitch(request_log_type_s, request_log_type_options, sizeof(request_log_type_options)/sizeof(request_log_type_options[0]), CASEFLAG_IGNORE_A);
		if (i > -1 && i < _REQUEST_LOG_END) {
			request_log_type = (REQUEST_LOG_TYPE)i;
			
			if (request_log_type == REQUEST_LOG_NONE)
				puts("[Config] Request logging disabled.");
			else
				printf("[Config] Using %s request logging.\n", log_prefixes[request_log_type]);
			
		} else {
			fprintf(stderr, "\x1b[31m[Config] Invalid request log type: '%s'\x1b[0m\n", request_log_type_s);
		}
	} else {
		fputs("\x1b[33m[Config] Warning: request log type not defined, setting to default: verbose\x1b[0m\n", stderr);
	}

	tls_setup(sconfig);
	
	sock = server_create_socket((uint16_t)strtoul(config_get(config, "port"), NULL, 0));
	socket_initialized = 1;
	
	/* post-init: */
	free(sconfig);
	config_destroy(config);
	
	/* This new line character is intentional ;) */
	puts("Initialization done.\n");
	
	/* Handle connections */
	uint32_t len = sizeof(struct sockaddr_in);
	while(1) {
		struct sockaddr_in addr;

		int client = accept(sock, (struct sockaddr*)&addr, &len);
		if (client < 0) {
			perror("Client acceptance failure");
			exit(EXIT_FAILURE);
		}
		
		/* enable TCP_NODELAY */
		int one = 1;
		setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (char *) &one, sizeof(int));

		TLS tls = tls_setup_client(client);

		if (tls) {
			/*
			const char *http_version, *http_version_minimal;
			*/
			TLS_AP ap = tls_get_ap(tls);
			switch (ap) {
				case TLS_AP_HTTP11: {
					/*
					http_version = "HTTP/1.1";
					http_version_minimal = "1.1";
					*/

					http_header_list_t *request = http1_parse(tls);
					if (request) {
						http_response_t *response = http_handle_request(request);
						http1_write_response(tls, response);

						if (response->is_dynamic) {
							free(response->body);
							http_response_headers_destroy(response->headers);
							free(response);
						}
						http_destroy_header_list(request);
					}
				} break;
				case TLS_AP_HTTP2:
					/*
					http_version = "HTTP/2";
					http_version_minimal = "2";
					*/
					http2_handle(tls);
					break;
				default:
					fputs("Invalid AP!\n", stderr);
					goto clean;
			}
			
			/*
			if (request) {
				http_response_t response = handle_request(*request);
				tls_write_client(tls, response.content, response.size == 0 ? strlen(response.content) : response.size);
				free(response.content);
				
				switch (request_log_type) {
					case REQUEST_LOG_MINIMAL:
						printf("> %s \"%s\" (%s) v=%s\n", request->method, request->path, http_common_log_status_names[response.status], http_version_minimal);
						break;
					case REQUEST_LOG_VERBOSE:
						printf("> path='%s' status='%s' method='%s' version='%s' response: %zi\n", request->path, http_common_log_status_names[response.status], request->method, http_version, response.size);
						break;
					default:
						break;
				}
			} else {
				if (request_log_type != REQUEST_LOG_NONE)
					puts("> Parser error");
			}
			*/
		}
		clean:
		close(client);
	}

	handle_destroy();
	close(sock);
	tls_destroy();
	
	return EXIT_SUCCESS;
}
 

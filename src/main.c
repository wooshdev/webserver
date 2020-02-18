/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "base/global_settings.h"
#include "client.h"
#include "configuration/config.h"
#include "handling/handlers.h"
#include "http/header_parser.h"
#include "http/http1.h"
#include "http/parser.h"
#include "http2/core.h"
#include "secure/tlsutil.h"
#include "server.h"
#include "utils/encoders.h"
#include "utils/fileutil.h"
#include "utils/io.h"
#include "utils/threads.h"
#include "utils/util.h"

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
		GLOBAL_SETTINGS_cancel_requested = 1;
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

	if (!encoder_setup()) {
		fputs("Failed to setup encoding algorithms!\n", stderr);
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

	GLOBAL_SETTINGS_load(config);
	
	if (!http_header_parser_setup(config_get(config, "compression"))) {
		fputs("Failed to setup HTTP header parser!\n", stderr);
		return EXIT_FAILURE;
	}

	/** static configuration options: **/
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
	while(!GLOBAL_SETTINGS_cancel_requested) {
		struct sockaddr_in addr;

		int client = accept(sock, (struct sockaddr*)&addr, &len);
		if (client < 0) {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				threads_yield_thread();
				continue;
			}
			
			perror("Unknown client acceptance failure");
			exit(EXIT_FAILURE);
		}
		
		int *data = malloc(sizeof(int));
		*data = client;
		
		client_start(data);
	}

	handle_destroy();
	close(sock);
	tls_destroy();
	GLOBAL_SETTINGS_destroy();
	encoder_destroy();
	http_header_parser_destroy();
	
	return EXIT_SUCCESS;
}
 

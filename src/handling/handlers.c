/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * TODO: Improve code documentation.
 */
#include "handlers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "base/global_settings.h"

#include "http/header_parser.h"
#include "utils/encoders.h"
#include "utils/util.h"


/*#include "fileserver.c"*/

static const char *H2_BODY_ok = "<body style=\"color:white;background:black;display:flex;align-items:center;width:100%;height:100%;justify-items:center;font-family:-apple-system,BlinkMacSystemFont,sans-serif;font-size:60px;text-align:center\"><h1>HTTP/2 now available!</h1></body>";
/* written some junk text below so I can see the effectiveness of encoders. */
static const char *H2_BODY_compression = "<!doctype html><html lang=\"en\"><head><meta name=\"description\" content=\"Main page for webserver.\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>webserver</title></head><body style=\"color:white;background:black;display:flex;align-items:center;width:100%;height:100%;justify-items:center;font-family:-apple-system,BlinkMacSystemFont,sans-serif;font-size:60px;text-align:center\"><h1>This content was encoded! Encoded content is faster than not-encoded content. This allows us to have greater performance. This performance is better.</h1></body></html>";
static const char *H2_BODY_not_found = "<h1>File Not Found.</h1>";
static const char *H2_BODY_method_not_supported = "<h1>Method not supported!</h1>";
static const char *HTTP_VERSION_NAMES[] = { "?", "h1", "h2" };

int handle_setup(config_t config) {
	const char *filenames_cfg = config_get(config, "handlers");
	if (!filenames_cfg) {
		fputs("[Handling] No 'handlers' field in configuration file.\n", stderr);
		return 0;
	}
	
	char *filenames = strdup(filenames_cfg);
	if (!filenames) {
		fputs("[Handling] Memory failure. :(\n", stderr);
		return 0;
	}
	char *component = strtok(filenames, " ");
	
	do {
		FILE *file = fopen(component, "r");
		if (!file) {
			fprintf(stderr, "[Handling] Invalid file: '%s'\n", component);
			free(filenames);
			return 0;
		}
		
		fclose(file);
	}	while ((component = strtok(0, " ")));
	
	free(filenames);
	return 1;
}

void handle_destroy() {
}

static int handle_write_length(http_response_headers_t *headers, size_t size) {
	/* TODO: Ensure length_buffer is big enough for size.
	 * unfortunately, I can't use snprintf because it isn't in POSIX '89/C89*/
	char *length_buffer = calloc(128, sizeof(char));
	int result = sprintf(length_buffer, "%zu", size);
	if (result <= 0) {
		printf("[Handlers] Failed to write length! Value=%zu\n", size);
		free(length_buffer);
		return 0;
	} else {
		char *lb_new = realloc(length_buffer, result * sizeof(char));
		if (!lb_new) {
			puts("[Handlers] Failed to reallocate length_buffer.");
			free(length_buffer);
			return 0;
		} else {
			int status = http_response_headers_add(headers, HTTP_RH_CONTENT_LENGTH, lb_new);
			free(lb_new);
			return status;
		}
	}
}

static void header_write_date(http_response_headers_t *headers) {
	char *value = calloc(128, sizeof(char));
	time_t the_time = time(NULL);
	size_t len = strftime(value, 128, "%a, %d %h %Y %T %z", localtime(&the_time));
	value = realloc(value, len + 1);
	http_response_headers_add(headers, HTTP_RH_DATE, value);
	free(value);
}

http_response_t *http_handle_request(http_header_list_t *request_headers, handler_callbacks_t *callbacks) {
	http_response_t *response = malloc(sizeof(http_response_t));
	response->is_dynamic = 1;
	size_t size = 0;

	const char *path = http_header_list_gets(request_headers, ":path");
	printf("[Handler] Path: '%s' (v: %s)\n", path, HTTP_VERSION_NAMES[request_headers->version]);

	const char *method = http_header_list_getd(request_headers, HEADER_METHOD);
	if (!method)
		method = http_header_list_gets(request_headers, ":method");
	if (!method || strcmp(method, "GET") != 0) {
		printf("[Handler] Unsupported method: %s\n", method);

		response->headers = http_create_response_headers(4);

		size_t size = strlen(H2_BODY_method_not_supported);
		response->body_size = size;

		http_response_headers_add(response->headers, HTTP_RH_STATUS_200, NULL);
		http_response_headers_add(response->headers, HTTP_RH_CONTENT_TYPE, "text/html; charset=UTF-8");
		http_response_headers_add(response->headers, HTTP_RH_SERVER, GLOBAL_SETTING_server_name);
		if (GLOBAL_SETTING_HEADER_sts)
			http_response_headers_add(response->headers, HTTP_RH_STRICT_TRANSPORT_SECURITY, GLOBAL_SETTING_HEADER_sts);
		handle_write_length(response->headers, size);
		
		if (callbacks && callbacks->headers_ready) {
			callbacks->headers_ready(response->headers, callbacks->application_data_length, callbacks->application_data);
		}

		response->body = strdup(H2_BODY_method_not_supported);
		return response;
	}
	
	/** Encoding **/
	compression_t compressor = COMPRESSION_TYPE_NONE;
	const char *accept_encoding_value = http_header_list_gets(request_headers, "accept-encoding");
	if (accept_encoding_value) {
		compressor = http_parse_accept_encoding(accept_encoding_value);
	}
	response->headers = http_create_response_headers(4);
	
	const char *possible_paths[] = { "/", "/favicon.ico", "/encoding" };

	switch (strswitch(path, possible_paths, sizeof(possible_paths) / sizeof(possible_paths[0]), CASEFLAG_DONT_IGNORE)) {
		case 0:
			http_response_headers_add(response->headers, HTTP_RH_STATUS_200, NULL);
			http_response_headers_add(response->headers, HTTP_RH_CONTENT_TYPE, "text/html; charset=UTF-8");
			response->body = strdup(H2_BODY_ok);
			size = strlen(H2_BODY_ok);
			break;
		case 1:
			http_response_headers_add(response->headers, HTTP_RH_STATUS_204, NULL);
			response->body = NULL;
			break;
		case 2:
			http_response_headers_add(response->headers, HTTP_RH_STATUS_200, NULL);
			http_response_headers_add(response->headers, HTTP_RH_CONTENT_TYPE, "text/html; charset=UTF-8");
			response->body = strdup(H2_BODY_compression);
			size = strlen(H2_BODY_compression);
			break;
		default:
			http_response_headers_add(response->headers, HTTP_RH_STATUS_404, NULL);
			http_response_headers_add(response->headers, HTTP_RH_CONTENT_TYPE, "text/html; charset=UTF-8");
			response->body = strdup(H2_BODY_not_found);
			size = strlen(H2_BODY_not_found);
			break;
	}
	header_write_date(response->headers);

#ifdef BENCHMARK
	clock_t start_time_compression = clock();
#endif
	if (size > 0) {
		encoded_data_t *encoded_data = NULL;
		if (compressor == COMPRESSION_TYPE_GZIP) {
			encoded_data = encode_gzip(response->body, size);
			http_response_headers_add(response->headers, HTTP_RH_CONTENT_ENCODING, "gzip");
		} else if (compressor == COMPRESSION_TYPE_BROTLI) {
			encoded_data = encode_brotli(response->body, size);
			http_response_headers_add(response->headers, HTTP_RH_CONTENT_ENCODING, "br");
		}

		if (encoded_data != NULL) {
			response->body = encoded_data->data;
			size = encoded_data->size;
			free(encoded_data);
		}
	} else {
		response->body = NULL;
		response->body_size = 0;
	}
#ifdef BENCHMARK
	printf("\033[0;33mCompression> \033[0;32m%s \033[0mtook \033[0;35m%.3f ms\033[0m to process...\n", response->headers->headers[response->headers->count-1]->value, (clock()-start_time_compression)/1000.0);
#endif

	handle_write_length(response->headers, size);
	http_response_headers_add(response->headers, HTTP_RH_SERVER, GLOBAL_SETTING_server_name);

	if (GLOBAL_SETTING_HEADER_sts)
		http_response_headers_add(response->headers, HTTP_RH_STRICT_TRANSPORT_SECURITY, GLOBAL_SETTING_HEADER_sts);
	if (GLOBAL_SETTING_HEADER_tk)
		http_response_headers_add(response->headers, HTTP_RH_TK, GLOBAL_SETTING_HEADER_tk);
	
	if (callbacks && callbacks->headers_ready) {
		callbacks->headers_ready(response->headers, callbacks->application_data_length, callbacks->application_data);
	}

	response->body_size = size;
	response->status = HTTP_LOG_STATUS_NO_ERROR;
	return response;
}

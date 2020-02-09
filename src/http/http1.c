/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * This file contains the main functions for HTTP/1.1.
 */
#include "http1.h"
#include <string.h>
#include "parser.h"
#include "../utils/io.h"
#include <stdlib.h>

const char *h1_last_line = "\r\n";

static char *compose_header_line(size_t *sbuffer, unsigned name, const char *value) {
	const char *key = http_rhnames[name];
	size_t skey = strlen(key);
	size_t svalue = strlen(value);
	*sbuffer = skey + svalue + 2;
	char *buffer = malloc(*sbuffer * sizeof(char));
	memcpy(buffer, key, skey);
	memcpy(buffer + skey, value, svalue);
	buffer[skey + svalue] = '\r';
	buffer[skey + svalue+1] = '\n';
	return buffer;
}

void http1_write_response(TLS tls, http_response_t *response) {
	/* is it better to put it all in one packet or not?
	   if not, using multiple tls_write_client's is
	   easier to code */
	size_t i;
	for (i = 0; i < response->headers->count; i++) {
		http_response_header_t *header = response->headers->headers[i];
		if (header->name >= HTTP_RH_STATUSES) {
			size_t sbuffer;
			char *buffer = compose_header_line(&sbuffer, header->name, header->value);
			tls_write_client(tls, buffer, sbuffer);
			free(buffer);
		} else {
			if (i != 0) {
				size_t sbuffer;
				char *buffer = compose_header_line(&sbuffer, header->name, header->value);
				buffer[sbuffer-3] = 0;
				printf("[HTTP/1.1] Warning: Status line not first header! Index=%zu, the first was: '%s'\n", i, buffer);
				free(buffer);
			}
			const char *line = http_rhnames[header->name];
			tls_write_client(tls, line, strlen(line));
		}
	}

	tls_write_client(tls, h1_last_line, 2);
	if (response->body_size && response->body) {
		tls_write_client(tls, response->body, response->body_size);
	}
}

http_header_list_t *http1_parse(TLS tls) {
	http_header_list_t *headers = http_create_header_list();
	if (!headers)
		return headers;
	headers->version = HTTP_VERSION_1;

	char *method = calloc(HTTP1_LONGEST_METHOD, sizeof(char));
	char *path = calloc(HTTP_PATH_MAX - 1, sizeof(char));
	char *version = calloc(HTTP_VERSION_MAX - 1, sizeof(char));

	if (!method || !path || !version)
		goto clean;
	
	
	/* parse method */
	if (!method || !http_parse_method(tls, method, HTTP1_LONGEST_METHOD)) {
		http_handle_error_gracefully(tls, HTTP_ERROR_UNSUPPORTED_METHOD, method, 0);
		goto clean;
	}
	
	/* parse path */
	if (io_read_until(tls, path, ' ', HTTP_PATH_MAX - 1) <= 0) {
		http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_PATH, path, 0);
		goto clean;
	}
	
	/* parse version */
	if (io_read_until(tls, version, '\r', HTTP_VERSION_MAX-1) <= 0 || strcmp(version, "HTTP/1.1")) {
		printf("invalid version='%s'\n", version);
		http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_VERSION, version, 0);
		goto clean;
	}
	
	http_header_list_add(headers, ":method", method, HTTP_HEADER_NAME_CACHED, 0);
	http_header_list_add(headers, ":path", path, HTTP_HEADER_NAME_CACHED, 0);
	
	/* remove the last '\n' character from the stream */
	char end_character[1];
	tls_read_client(tls, end_character, 1);
	
	size_t error = http_parse_headers(tls, headers);
	
	if (error) {
		static const char *header_errors[] = { 
			"HTTP_HEADER_PARSE_ERROR_NONE",
			"HTTP_HEADER_PARSE_ERROR_MEMORY",
			"HTTP_HEADER_PARSE_ERROR_READ",
			"HTTP_HEADER_PARSE_ERROR_MAX",
			"HTTP_HEADER_PARSE_ERROR_UNREGISTERED"
		};
		printf("[HTTP/1.1] Header Error: %s\n", header_errors[error]);
		goto clean;
	}
	
	const char *hostv;
	if (http_host_strict && (hostv = http_header_list_getd(headers, HEADER_AUTHORITY)) && strcmp(http_host, hostv)) {
		http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_HOST, version, 0);
		goto clean;
	}
	
	free(version); /* TODO */
	return headers;
	
	/* the ("clean") label is only used when things go wrong */
	clean:
	free(version); /* TODO */
	free(method);
	free(path);
	free(headers);
	return NULL;
}

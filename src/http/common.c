/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * For information about these functions/symbols, see common.h
 */
#include "common.h"

#include "../utils/io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define DATE_FORMATTED_SIZE 64

char http_header_server_name[128];
char http_host[128];

const char *http_common_log_status_names[] = { "?", "ok", "client error", "server error" };

static const char *error_statuses[] = { "405 Method Not Allowed", "414 URI Too Long", "505 HTTP Version Not Supported", "400 Bad Request" };
#define _MALFORMEDREQ "<h1>Your browser has sent a malformed request.</h1><hr><p>"
static const char *error_bodies[] = { 
	_MALFORMEDREQ"We can't support the sent <b>method</b> your browser wants to use.</p>", 
	_MALFORMEDREQ"We can't handle the <b>path</b> your browser sent us.</p>",
	_MALFORMEDREQ"We don't support the <b>version</b> your browser uses.</p>",
	_MALFORMEDREQ"The <b>host name</b> your browser has sent is incorrect.</p>"
};

static const char *supported_methods = "GET";

static char *get_date() {
    time_t t = time(NULL);
    struct tm *the_time = gmtime(&t);
    char *formatted = malloc(DATE_FORMATTED_SIZE);

    if (formatted == NULL)
        return NULL;

    size_t size = strftime(formatted, DATE_FORMATTED_SIZE, "%a, %c %b %G %T GMT", the_time);
    char *result = realloc(formatted, size+1);
    result[size] = '\0';

    return result;
}

void http_destroy_headers(http_headers_t headers) {
	size_t i;
	for (i = 0; i < headers.count; i++) {
		printf("destroy header: %zu/%zu %p %p\n", i, headers.count, headers.keys, headers.values);
		free(headers.keys[i]);
		free(headers.values[i]);
	}
}

void http_handle_error_gracefully(TLS source, HTTP_ERROR error, const char *information, int keep_alive) {
	if (sizeof(error_bodies) / sizeof(error_bodies[0]) <= error) {
		printf("[HTTP1x] Invalid graceful handling error=0x%x\n", error);
		return;
	}
	
	const char *format = "HTTP/1.1 %s\r\nStrict-Transport-Security: max-age=31536000; includeSubDomains; preload\r\nConnection: %s\r\nDate: %s\r\nAllow: %s\r\nServer: %s\r\nContent-Length: %s\r\n\r\n";
	/* the amount of times an argument char or list of chars has occurred, for example: '%s', '%zi' etc */
	size_t argument_chars = 10;
	
	const char *connection = keep_alive ? "keep-alive" : "close";
	char *date = get_date();
	const char *status = error_statuses[error];
	const char *body = error_bodies[error];
	
	/* content-length value */
	size_t body_size = strlen(body);
	FILE *null_fp = fopen("/dev/null", "w");
	char *content_length = malloc(fprintf(null_fp, "%zi", body_size) + 1);
	fclose(null_fp);
	sprintf(content_length, "%zi", body_size);
	
	size_t response_size = 
		/* format string - argument characters */
		strlen(format) - 
		argument_chars + 
		/* the header values*/
		strlen(status) +
		strlen(connection) + 
		strlen(date) + 
		strlen(http_header_server_name) +
		strlen(supported_methods) + 
		strlen(content_length);
	
	size_t buffer_size = response_size + body_size;
	char *buffer = malloc(buffer_size);
	size_t i;
	for (i = 0; i < buffer_size; i++)
		buffer[i] = 'A';
	
	sprintf(buffer, format, status, connection, date, supported_methods, http_header_server_name, content_length);
	strcpy(buffer + response_size, body);
	
	tls_write_client(source, buffer, buffer_size);
	
	free(content_length);
	free(date);
}

const char *http_get_header(http_headers_t headers, const char *key) {
	/* check to see if we should search in the list at all */
	if (!key || headers.count == 0)
		return NULL;
	
	size_t i;
	for (i = 0; i < headers.count; i++) {
		printf("%zi > %p %p\n", i, headers.keys[i], headers.values[i]);
		if (!strcmp(key, headers.keys[i])) {
			return headers.values[i];
		}
	}
	
	return NULL;
}

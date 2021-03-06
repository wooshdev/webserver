/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "response_headers.h"
#include "../utils/util.h"
#include <stdlib.h>
#include <stdio.h>

#define SIZE_INCREASE_STEP_SIZE 2

const char *http_rhnames[] = {
	"HTTP/1.1 200 OK\r\n",
	"HTTP/1.1 204 No Content\r\n",
	"HTTP/1.1 304 Not Modified\r\n",
	"HTTP/1.1 400 Bad Request\r\n",
	"HTTP/1.1 404 Not Found\r\n",
	"HTTP/1.1 500 Internal Server Error\r\n",
	"HTTP/1.1 503 Service Unavailable\r\n",
	"Content-Length: ",
	"Content-Type: ",
	"Date: ",
	"Server: ",
	"Tk: ",
	"Vary: ",
	"Content-Encoding: ",
	"Strict-Transport-Security: ",
	"Last-Modified: "
};

http_response_headers_t *http_create_response_headers(size_t initial_size) {
	http_response_headers_t *list = malloc(sizeof(http_response_headers_t));
	if (!list)
		return list;
	list->count = 0;
	list->size = initial_size;
	list->headers = calloc(initial_size, sizeof(http_response_header_t *));
	if (!list->headers) {
		free(list);
		return NULL;
	}
	return list;
}


void http_response_headers_destroy(http_response_headers_t *list) {
	size_t i;
	http_response_header_t *header;
	for (i = 0; i < list->count; i++) {
		if ((header = list->headers[i])) {
			free(header->value);
			free(header);
		}
	}
	free(list->headers);
	free(list);
}

int http_response_headers_add(http_response_headers_t *list, http_response_header_name name, const char *value) {
	if (!list) {
		puts("\x1b[31m[HTTPResponseHeaders] Error: HeaderList is null\x1b[0m");
		return 0;
	}

	/* resize if necessary */
	if (list->count + 1 == list->size) {
		http_response_header_t **headers = realloc(list->headers, (list->size += SIZE_INCREASE_STEP_SIZE) * sizeof(http_response_header_t *));
		if (!headers) {
			puts("\x1b[31m[HTTPResponseHeaders] Error: Header list reallocation error!\x1b[0m");
			return 0;
		}
		list->headers = headers;
	}

	http_response_header_t *header = calloc(1, sizeof(http_response_header_t));
	if (!header) {
		puts("\x1b[31m[HTTPResponseHeaders] Error: Header is null\x1b[0m");\
		return 0;
	}

	header->name = name;
	header->value = value ? strdup(value) : NULL;

	list->headers[list->count++] = header;
	return 1;
}

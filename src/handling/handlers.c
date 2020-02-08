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
#include "../utils/util.h"

/*#include "fileserver.c"*/

static const char *H2_BODY_ok = "<body style=\"color:white;background:black;display:flex;align-items:center;width:100%;height:100%;justify-items:center;font-family:-apple-system,BlinkMacSystemFont,sans-serif;font-size:60px;text-align:center\"><h1>HTTP/2 now available!</h1></body>";
static const char *H2_BODY_not_found = "<h1>File Not Found.</h1>";

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

http_response_t *http_handle_request(http_header_list_t *request_headers) {
	http_response_t *response = malloc(sizeof(http_response_t));
	response->is_dynamic = 1;
	size_t size = 0;
	
	const char *path = http_header_list_gets(request_headers, ":path");
	printf("[Handler] Path: %s\n", path);
	response->headers = http_create_response_headers(4);
	
	const char *possible_paths[] = { "/", "/favicon.ico" };
	
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
		default:
			http_response_headers_add(response->headers, HTTP_RH_STATUS_404, NULL);
			http_response_headers_add(response->headers, HTTP_RH_CONTENT_TYPE, "text/html; charset=UTF-8");
			response->body = strdup(H2_BODY_not_found);
			size = strlen(H2_BODY_not_found);
			break;
	}
	response->body_size = size;
	
	/* TODO: Ensure size is big enough */
	char *length_buffer = calloc(128, sizeof(char));
	sprintf(length_buffer, "%zu", size);
	
	http_response_headers_add(response->headers, HTTP_RH_CONTENT_LENGTH, length_buffer);
	http_response_headers_add(response->headers, HTTP_RH_SERVER, "TheWooshServer");
	
	response->status = HTTP_LOG_STATUS_NO_ERROR;
	return response;
	
}

http_response_t handle_request(http_request_t request) {
	http_response_t response = { 0 };
	
	/* MOVED TO http_handle_request! */
	
	return response;
}

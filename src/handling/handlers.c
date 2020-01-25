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

static char test[] = "HTTP/1.1 200 OK\r\nServer: wss\r\nConnection: close\r\nStrict-Transport-Security: max-age=31536000; includeSubDomains; preload\r\n\r\nStill working pls wait.";

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

http_response_t handle_request(http_request_t request) {
	http_response_t response = { 0 };
	
	char *message = strdup(test);
	
	size_t size = sizeof(test);
	response.content = message;
	response.size = size-1;
	
	response.status = HTTP_LOG_STATUS_NO_ERROR;
	return response;
}

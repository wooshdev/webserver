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
#include <strings.h>
#include <time.h>

#include "base/global_settings.h"
#include "configuration/config.h"
#include "http/header_parser.h"
#include "utils/encoders.h"
#include "utils/util.h"

#include "handler_utils.c"
#include "fallback_responses.c"
#include "fileserver.c"

static const char *handler_options[] = { 
	/* general: */
	"name", "type", "web-root", "overwrite-header",
	/* fileserver specific: */
	"directory", "send-modified"
};
static const char *handler_types[] = { "NONE", "fileserver" };

size_t handler_count;
http_handler_t **handlers;

int handle_setup(config_t main_config) {
	if (handlers) {
		fputs("handler_setup() called twice!\n", stderr);
		return 0;
	}
	handler_count = 0;
	
	const char *filenames_cfg = config_get(main_config, "handlers");
	if (!filenames_cfg) {
		fputs("[Handling] No 'handlers' field in configuration file.\n", stderr);
		return 0;
	}
	
	char *filenames = strdup(filenames_cfg);
	if (!filenames) {
		fputs("[Handling] Memory failure. :(\n", stderr);
		return 0;
	}

	size_t i, j, handlers_size = 1;
	for (i = 0; i < strlen(filenames); i++) {
		if (filenames[i] == ' ')
			handlers_size++;
	}
	handlers = calloc(handlers_size, sizeof(http_handler_t *));

	char *component = strtok(filenames, " ");

	http_handler_t *handler = NULL;
	config_t config;

	do {
		FILE *file = fopen(component, "r");
		if (!file) {
			fprintf(stderr, "[Handling] Invalid file: '%s'\n", component);
			free(filenames);
			return 0;
		}

		handler = malloc(sizeof(http_handler_t));
		if (!handler) {
			free(filenames);
			puts("[Handlers] handle_setup: malloc error.");
			return 0;
		}

		memset(handler, 0, sizeof(http_handler_t));
		config = config_readf(file);

		size_t overwrite_headers = 0;
		for (i = 0; i < config.count; i++)
			if (strcasecmp(config.keys[i], "overwrite-header") == 0)
				overwrite_headers += 1;

		if (overwrite_headers > 0) {
			handler->overwrite_headers_names = calloc(overwrite_headers, sizeof(char *));
			handler->overwrite_headers_values = calloc(overwrite_headers, sizeof(char *));
		}

		for (i = 0; i < config.count; i++) {
			switch (strswitch(config.keys[i], handler_options, sizeof(handler_options)/sizeof(handler_options[0]), CASEFLAG_IGNORE_B)) {
				case 0: /* "name" */
					if (handler->name) {
						printf("[Handler] Duplicate name option: first=\"%s\" second=\"%s\"! File name: \"%s\"\n", handler->name, config.values[i], component);
					} else if (strlen(config.values[i]) == 0) {
						printf("[Handler] Handler name cannot be empty! File name: %s\n", component);
					} else if (!(handler->name = strdup(config.values[i]))) {
						printf("[Handler] Memory error for name=\"%s\" file=\"%s\"!\n", config.values[i], component);
					} else {
						goto forlooplabel;
					}

					goto error_all;
				case 1: /* "type" */
					if (handler->type != HTTP_HANDLER_TYPE_NONE) {
						printf("[Handler] Handler type defined twice! File name: \"%s\"\n", component);
					} else if (strlen(config.values[i]) == 0) {
						printf("[Handler] Handler types cannot be empty! File name: %s\n", component);
					} else {
						switch (strswitch(config.values[i], handler_types, sizeof(handler_types) / sizeof(handler_types[0]), CASEFLAG_IGNORE_B)) {
							case 1:
								handler->type = HTTP_HANDLER_TYPE_FILESERVER;
								handler->data = malloc(sizeof(handler_fs_t));
								if (!handler->data) {
									printf("[Handler] Memory error on fileserver data creation! File name: \"%s\"!\n", component);
								}
								/* default settings: */
								((handler_fs_t *)handler->data)->send_mod = 1;
								break;
							default:
								printf("[Handler] Invalid handler type: '%s'! File name: %s\n", config.values[i], component);
								goto error_all;
						}
						
						goto forlooplabel;
					}
					goto error_all;
				case 2: /* "web-root" */
					if (handler->root) {
						printf("[Handler] Handler web-root defined twice! File name: \"%s\"\n", component);
					} else if (strlen(config.values[i]) == 0) {
						printf("[Handler] Handler 'web-root's cannot be empty! File name: %s\n", component);
					} else if (!(handler->root = strdup(config.values[i]))) {
						printf("[Handler] Memory error for web-root=\"%s\" file=\"%s\"!\n", config.values[i], component);
					} else {
						goto forlooplabel;
					}

					goto error_all;
				case 3: {/* "overwrite-header" */
					char *space_char = strchr(config.values[i], ' ');
					if (!space_char) {
						printf("[Handler] Overwrite-header's name and value should be seperated by a space. Value: \"%s\". File name: \"%s\"\n", config.values[i], component);
						goto error_all;
					}

					*space_char = 0;
					if (!(handler->overwrite_headers_names[handler->overwrite_header_count] = strdup(config.values[i])) || !(handler->overwrite_headers_values[handler->overwrite_header_count] = strdup(space_char + 1))) {
						*space_char = ' ';
						printf("[Handler] Overwrite-header memory error! Value: \"%s\". File name: \"%s\"\n", config.values[i], component);
						goto error_all;
					}

					break;
				}
				case 4: {/* "directory" */
					if (handler->type != HTTP_HANDLER_TYPE_FILESERVER || !handler->data) {
						printf("[Handler] The directory option can only be set on fileservers, not %ss! File name: \"%s\"\n", handler_types[handler->type], component);
						goto error_all;
					}

					if (strlen(config.values[i]) == 0) {
						printf("[Handler] FileServers directory cannot be empty! File name: %s\n", component);
						goto error_all;
					}

					handler_fs_t *fs = (handler_fs_t *) handler->data;
					if (!(fs->wdir = strdup(config.values[i]))) {
						printf("[Handler] FileServerDirectory MemoryError! Value: \"%s\". File name: \"%s\"\n", config.values[i], component);
						goto error_all;
					}

					break;
				}
				case 5: {/* "send-modified" */
					if (handler->type != HTTP_HANDLER_TYPE_FILESERVER || !handler->data) {
						printf("[Handler] The send-modified option can only be set on fileservers, not %ss! File name: \"%s\"\n", handler_types[handler->type], component);
						goto error_all;
					}

					handler_fs_t *fs = (handler_fs_t *) handler->data;
					fs->send_mod = config_get_bool(config, "send-modified", 1);

					break;
				}
				default:
					printf("Warning: Unknown property: \"%s\" with value \"%s\"\n", config.keys[i], config.values[i]);
					break;
			}

			/* A workaround for the missing continue for-loop feature. */
			forlooplabel:;
		}

		/* Validation */
		if (!handler->name) {
			printf("Error: Handler doesn't have the 'name' property set! File: %s\n", component);
		} else if (handler->type == HTTP_HANDLER_TYPE_NONE) {
			printf("Error: Handler doesn't have the 'type' property set! File: '%s', Name: '%s'\n", component, handler->name);
		} else if (!handler->root) {
			printf("Error: Handler doesn't have the 'root' property set! File: '%s', Name: '%s'\n", component, handler->name);
		} else if (handler->type == HTTP_HANDLER_TYPE_FILESERVER && (!handler->data || !((handler_fs_t *) handler->data)->wdir)) {
			printf("Error: FileServer doesn't have the 'directory' property set! File: '%s', Name: '%s'\n", component, handler->name);
		} else {
			goto continue_with_loop;
		}
		puts("DEBUG: Validation Error!");
		goto error_all; /* TODO This is ugly. */

		continue_with_loop:
		handlers[handler_count++] = handler;
		config_destroy(config);
	}	while ((component = strtok(0, " ")));
	
	free(filenames);
	setup_responses();
	return 1;

error_all:
	if (handler_count > 0)
		for (j = 0; j < handler_count; j++)
			destroy_handler(handlers[j]);

	destroy_handler(handler);
	free(filenames);
	config_destroy(config);
	free(handlers);
	return 0;
}

void handle_destroy() {
	size_t i;
	for (i = 0; i < handler_count; i++) {
		destroy_handler(handlers[i]);
	}
	free(handlers);
}

http_response_t *http_handle_request(http_header_list_t *request_headers, handler_callbacks_t *callbacks) {

	const char *path = http_header_list_gets(request_headers, ":path");
	if (!path) {
		puts("DEBUG: client didn't send a path!");
		if (callbacks && callbacks->headers_ready)
			callbacks->headers_ready(response_invalid_request->headers, callbacks->application_data_length, callbacks->application_data);
		return response_invalid_request;
	}

	size_t i;
	http_handler_t *handler;
	for (i = 0; i < handler_count; i++) {
		handler = handlers[i];
		if (strstartsw(path, handler->root)) {
			http_response_t *response = NULL;
			switch (handler->type) {
				case HTTP_HANDLER_TYPE_FILESERVER:
					response = fs_handle(path, handler, request_headers, callbacks);
					break;
				default:
					printf("DEBUG: Unknown handler type=%u\n", handler->type);
					break;
			}

			if (response)
				return response;
			puts("DEBUG: Warning: handler couldn't process request!");
		}
	}

	puts("DEBUG: Warning no handler!");
	http_response_t *response = response_no_service;
	if (callbacks && callbacks->headers_ready) {
		callbacks->headers_ready(response->headers, callbacks->application_data_length, callbacks->application_data);
	}

	return response;
}

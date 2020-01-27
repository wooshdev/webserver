/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * TODO: Improve code documentation.
 */
#ifndef HANDLERS_H
#define HANDLERS_H

#include "../http/common.h"

typedef http_response_t (*http_handler_func)(http_request_t);

typedef enum HTTP_HANDLER_TYPE {
	HTTP_HANDLER_TYPE_NONE = 0x0,
	HTTP_HANDLER_TYPE_FILESERVER = 0x1,
} HTTP_HANDLER_TYPE;

typedef struct handler_fs_t {
	/* working directory */
	char *wdir;
	/* (boolean) send modification date */
	int send_mod;
} handler_fs_t;

typedef struct http_handler_t {
	HTTP_HANDLER_TYPE type;
	char *name;
	char *root;
	http_handler_func function;
	/* user data */
	void *data;
} http_handler_t;

/**
 * Description:
 *   This function sets up the handlers, based on the configuration file.
 *
 * Parameters:
 *   config_t
 *     The configuration file to read from.
 *
 * Return Value:
 *   (boolean) success status
 */
int handle_setup(config_t);

void handle_destroy(void);

/**
 * Description:
 *   This function will forward the request to the correct handler.
 *
 * Parameters:
 *   http_request_t
 *     The request in parsed form.
 *
 * Return Value:
 *   The HTTP response. The content field can be NULL, but the error field should be set.
 */
http_response_t handle_request(http_request_t);

size_t handler_count;
http_handler_t **handlers;


#endif /* HANDLERS_H */
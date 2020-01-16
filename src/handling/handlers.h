#ifndef HANDLERS_H
#define HANDLERS_H

#include "../http/common.h"

typedef http_response_t (*http_handler_func)(http_request_t);

typedef enum HTTP_HANDLER_TYPE {
	HTTP_HANDLER_TYPE_FILE_SYSTEM = 0x0,
} HTTP_HANDLER_TYPE;

typedef struct http_handler_t {
	HTTP_HANDLER_TYPE type;
	const char *name;
	const char *root;
	http_handler_func function;
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

#endif /* HANDLERS_H */

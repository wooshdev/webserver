/**
 * Copyright (C) 2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * TODO: Improve code documentation.
 */

static http_response_t *response_invalid_request;
static http_response_t *response_no_service;

static const char *response_body_invalid_request = "<!doctype html><html lang=\"en\"><head><title>Invalid Request</title></head><body><h1>Invalid Request</h1></body></html>";
static const char *response_body_no_service = "<!doctype html><html lang=\"en\"><head><title>Service Unavailable</title><style>*{font-family:sans-serif}</style></head><body><h1>HTTP Error 503: Service Unavailable</h1><hr><p>If you are the administrator of this server, please see your log files and check your configuration. Explanation: no handler was configured to handle this path and no error handlers were setup.</body></html>";
static const char *response_body_fs_not_found = "<!doctype html><html lang=\"en\"><head><title>Not Found</title><style>*{font-family:sans-serif}</style></head><body><h1>HTTP Error 404: Not Found</h1><hr><p>If you are the administrator of this server, please see your log files and check your configuration. Explanation: no handler was configured to handle this path and no error handlers were setup.</body></html>";

static void setup_responses(void) {
	response_invalid_request = malloc(sizeof(http_response_t));
	response_invalid_request->is_dynamic = 0;
	response_invalid_request->headers = http_create_response_headers(5);
	http_response_headers_add(response_invalid_request->headers, HTTP_RH_STATUS_400, NULL);
	http_response_headers_add(response_invalid_request->headers, HTTP_RH_CONTENT_TYPE, "text/html; charset=UTF-8");
	size_t size = strlen(response_body_invalid_request);
	handle_write_length(response_invalid_request->headers, size);
	http_response_headers_add(response_invalid_request->headers, HTTP_RH_SERVER, GLOBAL_SETTING_server_name);
	if (GLOBAL_SETTING_HEADER_sts)
		http_response_headers_add(response_invalid_request->headers, HTTP_RH_STRICT_TRANSPORT_SECURITY, GLOBAL_SETTING_HEADER_sts);
	response_invalid_request->body = strdup(response_body_invalid_request);
	response_invalid_request->body_size = size;

	response_no_service = malloc(sizeof(http_response_t));
	response_no_service->is_dynamic = 0;
	response_no_service->headers = http_create_response_headers(5);
	http_response_headers_add(response_no_service->headers, HTTP_RH_STATUS_503, NULL);
	http_response_headers_add(response_no_service->headers, HTTP_RH_CONTENT_TYPE, "text/html; charset=UTF-8");
	size = strlen(response_body_no_service);
	handle_write_length(response_no_service->headers, size);
	http_response_headers_add(response_no_service->headers, HTTP_RH_SERVER, GLOBAL_SETTING_server_name);
	if (GLOBAL_SETTING_HEADER_sts)
		http_response_headers_add(response_no_service->headers, HTTP_RH_STRICT_TRANSPORT_SECURITY, GLOBAL_SETTING_HEADER_sts);
	response_no_service->body = strdup(response_body_no_service);
	response_no_service->body_size = size;
}

static void destroy_handler(http_handler_t *handler) {
	if (handler->overwrite_header_count > 0) {
		size_t i;
		for (i = 0; i < handler->overwrite_header_count; i++) {
			free(handler->overwrite_headers_names[i]);
			free(handler->overwrite_headers_values[i]);
		}
		free(handler->overwrite_headers_names);
		free(handler->overwrite_headers_values);
	}

	free(handler->name);
	free(handler->root);
	free(handler->data);
	free(handler);
}

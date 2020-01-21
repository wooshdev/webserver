#include "http1.h"
#include <string.h>
#include "common.h"
#include "parser.h"
#include "../utils/io.h"
#include <stdlib.h>

http_request_t *http1_parse(TLS tls) {
	http_request_t *request = calloc(1, sizeof(http_request_t));
	
	request->method = calloc(HTTP1_LONGEST_METHOD, sizeof(char));

	/* parse method */
	if (!request->method || !http_parse_method(tls, request->method, HTTP1_LONGEST_METHOD) || /* only support 'GET' atm. */ strcmp(request->method, "GET")) {
		http_handle_error_gracefully(tls, HTTP_ERROR_UNSUPPORTED_METHOD, request->method, 0);
		goto clean;
	}
	
	/* parse path */
	request->path[HTTP_PATH_MAX - 1] = 0;
	if (io_read_until(tls, request->path, ' ', HTTP_PATH_MAX-1) <= 0) {
		http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_PATH, request->path, 0);
		goto clean;
	}
	
	/* parse version */
	request->version[HTTP_VERSION_MAX - 1] = 0;
	if (io_read_until(tls, request->version, '\r', HTTP_VERSION_MAX-1) <= 0 || strcmp(request->version, "HTTP/1.1")) {
		printf("invalid version='%s'\n", request->version);
		http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_VERSION, request->version, 0);
		goto clean;
	}
	
	/* remove the last '\n' character from the stream */
	char end_character[1];
	tls_read_client(tls, end_character, 1);
	
	memset(&request->headers, 0, sizeof(request->headers));
	http_parse_headers(tls, request->headers);
	
	if (request->headers.error != HTTP_HEADER_PARSE_ERROR_NONE) {
		static const char *header_errors[] = { "HTTP_HEADER_PARSE_ERROR_NONE", "HTTP_HEADER_PARSE_ERROR_IO", "HTTP_HEADER_PARSE_ERROR_MAX", "HTTP_HEADER_PARSE_ERROR_UNREGISTERED" };
		printf("[HTTP/1.1] Header Error: %s\n", header_errors[request->headers.error]);
		goto clean;
	}
	
	const char *hostv;
	if (http_host_strict && (hostv = http_get_header(request->headers, "host")) && strcmp(http_host, hostv)) {
		http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_HOST, request->version, 0);
		goto clean;
	}
	
	return request;
	
	clean:
	free(request);
	return NULL;
}

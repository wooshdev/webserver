#include "http1.h"
#include <string.h>
#include "common.h"
#include "parser.h"
#include "../utils/io.h"
#include <stdlib.h>

http_request_t *http1_parse(TLS tls) {
	http_request_t *request = calloc(1, sizeof(http_request_t));
	
	http_request_t req = *request;
	req.method = calloc(HTTP1_LONGEST_METHOD, sizeof(char));
	
	puts("Using HTTP/1.1");

	/* parse method */
	if (!req.method || !http_parse_method(tls, req.method, HTTP1_LONGEST_METHOD) || /* only support 'GET' atm. */ strcmp(req.method, "GET")) {
		http_handle_error_gracefully(tls, HTTP_ERROR_UNSUPPORTED_METHOD, req.method, 0);
		goto clean;
	}
	
	/* parse path */
	req.path[HTTP_PATH_MAX - 1] = 0;
	if (io_read_until(tls, req.path, ' ', HTTP_PATH_MAX-1) <= 0) {
		http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_PATH, req.path, 0);
		goto clean;
	}
	
	/* parse version */
	req.version[HTTP_VERSION_MAX - 1] = 0;
	if (io_read_until(tls, req.version, '\r', HTTP_VERSION_MAX-1) <= 0 || strcmp(req.version, "HTTP/1.1")) {
		printf("invalid version='%s'\n", req.version);
		http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_VERSION, req.version, 0);
		goto clean;
	}
	
	/* remove the last '\n' character from the stream */
	char end_character[1];
	tls_read_client(tls, end_character, 1);
	
	memset(&request->headers, 0, sizeof(request->headers));
	http_parse_headers(tls, request->headers);
	/*printf("header_error=%i\n", headers.error);*/
	
	const char *hostv;
	if (http_host_strict && (hostv = http_get_header(req.headers, "host")) && strcmp(http_host, hostv)) {
		http_handle_error_gracefully(tls, HTTP_ERROR_INVALID_HOST, req.version, 0);
		goto clean;
	}
	
	return request;
	
	clean:
	free(request);
	return NULL;
}

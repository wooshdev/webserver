/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * This file contains the symbols for response header lists.
 * These are header lists optimized for responses, since they 
 * only need to be parsed/sent back to the client. This creates
 * an opportunity to remove the header keys as strings and 
 * replace them with a faster method; enums.
 */
#ifndef HTTP_RESPONSE_HEADERS_H
#define HTTP_RESPONSE_HEADERS_H

#include <stddef.h>

/* The amount of statuses in http_response_header_name.
 * This should also be the same as the offset of the 
 * first "normal" header, since statuses should be at 
 * the end as they are special; they are not headers in
 * HTTP < 2 */
#define HTTP_RH_STATUSES 7

typedef enum {
	HTTP_RH_STATUS_200,
	HTTP_RH_STATUS_204,
	HTTP_RH_STATUS_304,
	HTTP_RH_STATUS_400,
	HTTP_RH_STATUS_404,
	HTTP_RH_STATUS_500,
	HTTP_RH_STATUS_503,
	HTTP_RH_CONTENT_LENGTH,
	HTTP_RH_CONTENT_TYPE,
	HTTP_RH_DATE,
	HTTP_RH_SERVER,
	HTTP_RH_TK,
	HTTP_RH_VARY,
	HTTP_RH_CONTENT_ENCODING,
	HTTP_RH_STRICT_TRANSPORT_SECURITY,
	HTTP_RH_LAST_MODIFIED
} http_response_header_name;

/* These are the text representation for HTTP/1.1 of the enums above,
	for example: { "Content-Length: ", "Content-Type: " } etc. */
extern const char *http_rhnames[];

typedef struct {
	http_response_header_name name;
	char *value;
} http_response_header_t;

typedef struct {
	http_response_header_t **headers;
	size_t count;
	/* Important: size != count */
	size_t size;
} http_response_headers_t;

/**
 * Description:
 *   Creates and sets up an empty header list.
 *
 * Parameters:
 *   size_t
 *     The initial (estimate) size.
 *
 * Return Value:
 *   An empty list as advertised by the description, 
 *   or NULL if an I/O error has occurred.
 */
http_response_headers_t *http_create_response_headers(size_t);

/**
 * Description:
 *   Destroys the header list. 
 *   This will also free all values.
 *
 * Return Value:
 *   See description.
 */
void http_response_headers_destroy(http_response_headers_t *);

/**
 * Description:
 *   Adds a header to the list.
 *
 * Parameters:
 *   http_response_headers_t *
 *     The header list to add the header to.
 *   http_response_header_name
 *     The name of the header. (Also known as the key of the header)
 *   const char *
 *     The header value. 
 *     This value will be duplicated but the duplicated value will be freed by http_response_headers_destroy
 *
 * Return Value:
 *   (Boolean) Success Status
 */
int http_response_headers_add(http_response_headers_t *, http_response_header_name, const char *);

#endif /* HTTP_RESPONSE_HEADERS_H */

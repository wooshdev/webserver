/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * For information about these functions/symbols, see parser.h
 */
#include "parser.h"

#include "../utils/io.h"

#include <stdio.h>
#include <string.h>

/**
 * The list of valid HTTP/1x methods.
 *
 * Source:
 *   https://www.iana.org/assignments/http-methods/http-methods.xhtml
 *
 * Last Updated:
 *   In code:
 *     1/12/19
 *   By IANA:
 *     2017-04-14
 */
static const char *iana_methods[] = { "ACL", "BASELINE-CONTROL", "BIND", "CHECKIN", "CHECKOUT", "CONNECT", "COPY", "DELETE", "GET", "HEAD", "LABEL", "LINK", "LOCK", "MERGE", "MKACTIVITY", "MKCALENDAR", "MKCOL", "MKREDIRECTREF", "MKWORKSPACE", "MOVE", "OPTIONS", "ORDERPATCH", "PATCH", "POST", "PRI", "PROPFIND", "PROPPATCH", "PUT", "REBIND", "REPORT", "SEARCH", "TRACE", "UNBIND", "UNCHECKOUT", "UNLINK", "UNLOCK", "UPDATE", "UPDATEREDIRECTREF", "VERSION-CONTROL" };

int http_parser_setup() {
	/* determine the max method length */
	size_t method_max = 0, method_len, i;
	
	for (i = 0; i < sizeof(iana_methods) / sizeof(iana_methods[0]); i++) {
		method_len = strlen(iana_methods[i]);
		
		if (method_len > method_max)
			method_max = method_len;
	}
	HTTP1_LONGEST_METHOD = method_max + 1;
	
	
	return 1;
}

int http_parse_method(TLS source, char *dest, size_t size) {
	/* read from TLS */
	if (io_read_until(source, dest, ' ', size) <= 0) {
		return 0;
	}
	
	/* null-terminate*/
	dest[size - 1] = 0;
	
	if (!strlen(dest)) {
		/* maybe warn about invalid method? */
		return 0;
	}
	
	size_t i;
	for (i = 0; i < sizeof(iana_methods) / sizeof(iana_methods[0]); i++) {
		if (!strcmp((const char *) dest, iana_methods[i])) {
			return 1;
		}
	}
	
	printf("[HTTP/1x] [Parser] Invalid HTTP/1.1 method: '%s'\n", dest);
	return 0;
}

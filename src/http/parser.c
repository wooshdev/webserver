/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * For information about these functions/symbols, see parser.h
 */
#include "parser.h"

#include "../utils/io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

http_headers_t http_parse_headers(TLS tls) {
	http_headers_t headers = { 0 };
	
	char key_buffer[HTTP_HEADERS_KEY_MAX_LENGTH];
	char value_buffer[HTTP_HEADERS_VALUE_MAX_LENGTH];
	
	int read;
	while ((read = io_read_until(tls, key_buffer, ':', HTTP_HEADERS_KEY_MAX_LENGTH)) > 0) {
		char *key = malloc(sizeof(char) * read);
    if (!key)
      break;
    
		key[read] = 0; /* NULL-terminate */
		strcpy(key, key_buffer); /* copy buffer */
    
    /* lowercase the key */
    size_t i;
    for(i = 0; key[i]; i++){
      key[i] = tolower(key[i]);
    }
    
    headers.keys[headers.count] = key;
    
    /* read header value */
    if ((read = io_read_until(tls, value_buffer, '\n', HTTP_HEADERS_KEY_MAX_LENGTH)) <= 0)  {
      free(key);
      headers.error = HTTP_HEADER_PARSE_ERROR_IO;
      return headers;
    }
    
    /* remove the last char; '\r' as the line ends with \r\n */
    value_buffer[read-1] = 0;
    
    char *value = malloc(sizeof(char) * read);
    if (!value) {
      free(key);
      break;
    }
    strcpy(value, value_buffer+1);
    
    /* put value in map */
    headers.values[headers.count] = value;
    
    
    if (++headers.count > HTTP_HEADERS_MAX) {
      puts("Client sent too many headers!");
      headers.error = HTTP_HEADER_PARSE_ERROR_MAX;
      return headers;
    }
	}
	
	headers.error = HTTP_HEADER_PARSE_ERROR_IO;
	return headers;
}
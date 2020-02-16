/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains symbols concerning common utilities, such as error-handling.
 */
#ifndef HTTP_COMMON_H
#define HTTP_COMMON_H

#include "../secure/tlsutil.h"
#include "response_headers.h"

#define HTTP_PATH_MAX 2048
#define HTTP_VERSION_MAX 10 /* 'http/1.1'+CR+NULL is the longest */
#define HTTP_HEADERS_KEY_MAX_LENGTH 64
#define HTTP_HEADERS_VALUE_MAX_LENGTH 1024
#define HTTP_HEADERS_MAX 64

/**
 * Description:
 *   Should the header parser be strict? I.e. only allow for IANA registered headers.
 * 
 * Value:
 *   (boolean)
 */
int http_headers_strict;
int http_host_strict;

typedef enum HTTP_ERROR {
	/* The method the client specified was:
	 * 1. (As string) too long
	 * 2. Not registered (IANA)
	 */
	HTTP_ERROR_UNSUPPORTED_METHOD = 0x00,
	/* The path the client specified was probably too long, see HTTP_PATH_MAX
	 */
	HTTP_ERROR_INVALID_PATH = 0x01,
	/* The path the client specified was:
	 * 1. Invalid as string (too long, I/O, etc)
	 * 2. Not equal to 'HTTP/1.1'
	 */
	HTTP_ERROR_INVALID_VERSION = 0x02,
	/**
	 * The host is not equal to the one defined in the configuration, and hostname-strict is set to true.
	 */
	HTTP_ERROR_INVALID_HOST = 0x03
} HTTP_ERROR;

typedef enum HTTP_HEADER_PARSE_ERROR {
	HTTP_HEADER_PARSE_ERROR_NONE = 0x0,
	HTTP_HEADER_PARSE_ERROR_IO = 0x1,
	HTTP_HEADER_PARSE_ERROR_MAX = 0x2,
	HTTP_HEADER_PARSE_ERROR_UNREGISTERED = 0x3,
} HTTP_HEADER_PARSE_ERROR;

typedef enum HTTP_HANDLE_ERROR {
	HTTP_HANDLE_ERROR_NONE = 0x0
} HTTP_HANDLE_ERROR;

typedef enum {
	/* This is the initial value of the status (this should be changed) */
	HTTP_LOG_STATUS_INVALID = 0x0,
	HTTP_LOG_STATUS_NO_ERROR = 0x1,
	HTTP_LOG_STATUS_CLIENT_ERROR = 0x2,
	HTTP_LOG_STATUS_SERVER_ERROR = 0x3
} HTTP_LOG_STATUS;

typedef struct http_headers_t {
	size_t count;
	char *keys[HTTP_HEADERS_MAX];
	char *values[HTTP_HEADERS_MAX];
	HTTP_HEADER_PARSE_ERROR error;
} http_headers_t;

typedef struct http_response_t {
	/** 
	 * Should  we free this response and its contents 
	 * after sending it to the client?   Setting this 
	 * to false enables us to cache responses which 
	 * results in faster load times. */
	int is_dynamic;
	
	http_response_headers_t *headers;
	size_t body_size;
	/* "body" will be freed. */
	char *body;
	
	/* The status of the response. This is purely used for logging. */
	HTTP_LOG_STATUS status;
	
	/* The error, or HTTP_HANDLE_ERROR_NONE if none */
	HTTP_HANDLE_ERROR error;
} http_response_t;

typedef struct http_request_t {
	char *method;
	char path[HTTP_PATH_MAX];
	char version[HTTP_VERSION_MAX];
	http_headers_t headers;
} http_request_t;

/**
 * Description:
 *   This function will handle the request gracefully, which means that it will send a full 
 *   request, with some 4xx or 5xx error status, required headers and an optional body.
 * 
 * Parameters:
 *   TLS
 *     The TLS source to be read from.
 *   HTTP_ERROR
 *     The error.
 *   const char *
 *     (Nullable) Optional information regarding the error.
 *   int
 *     (boolean) 0 for 'Connection' header to be 'close', otherwise 'keep-alive'.
 */
void http_handle_error_gracefully(TLS, HTTP_ERROR, const char *, int);
/**
 * Description:
 *   This function will destroy any date from the map.
 * 
 * Parameters:
 *   http_headers_t
 *     The headers to be destroyed.
 */
void http_destroy_headers(http_headers_t);

/**
 * Description:
 *   Gets the value of the header.
 * 
 * Notes:
 *   The key should be lowercased
 * 
 * Parameters:
 *   http_headers_t
 *     The header map.
 *   const char *
 *     The header key.
 *
 * Return Value:
 *   A valid string, or NULL if the header wasn't found.
 */
const char *http_get_header(http_headers_t, const char *);

#endif /* HTTP_COMMON_H */
 

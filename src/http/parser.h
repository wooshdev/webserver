/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains symbols concerning the parsing of HTTP/1.1 bytes.
 */
#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "../secure/tlsutil.h"

/**
 * Description:
 *   The longest string length of all the valid HTTP/1x methods.
 *
 * String Properties:
 *   - Including null-terminator
 */
size_t HTTP1_LONGEST_METHOD;

/**
 * Description:
 *   This function will setup the parser.
 * 
 * Return value:
 *   (boolean) success status
 */
int http_parser_setup();

/**
 * Description:
 *   This function will parse the HTTP/1.1 method and will check the validity of the method.
 * 
 * Parameters:
 *   TLS
 *     The TLS source to be read from.
 *   char *
 *     The destination buffer.
 *   size_t
 *     The maximum bytes to be read.
 * 
 * Return value:
 *   -1 invalid/illegal method
 *    0 I/O failure
 *    1 success
 */
int http_parse_method(TLS, char *, size_t);

#endif /* HTTP_PARSER_H */

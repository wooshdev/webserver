/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * This file contains symbols helping with the parsing of header values.
 */
#ifndef HTTP_HEADER_PARSER_H
#define HTTP_HEADER_PARSER_H

/** Structures **/
typedef enum {
	COMPRESSION_TYPE_ERROR = 0x0,
	COMPRESSION_TYPE_NONE = 0x1,
	COMPRESSION_TYPE_GZIP = 0x2,
	COMPRESSION_TYPE_BROTLI = 0x3,
	COMPRESSION_TYPE_ANY = 0x4,
} compression_t;

/** Global Variables **/
extern compression_t *hp_compressors;

/** Functions **/
/**
 * Description:
 *   This function parses the header value of the 'Accept-Encoding' header.
 *
 * Parameters:
 *   const char *
 *     The value of the Accept-Encoding header.
 *
 * Return Value:
 *   The best available compression type.
 */
compression_t http_parse_accept_encoding(const char *);

/**
 * Description:
 *   This function sets the parser up.
 *
 * Parameters:
 *   const char *
 *     The value of the "compression" config option.
 *
 * Return Value:
 *   (boolean) Success Status
 */
int http_header_parser_setup(const char *);

/**
 * Description:
 *   This function destroys all data allocated by http_header_parser_setup.
 */
void http_header_parser_destroy(void);

#endif /* HTTP_HEADER_PARSER_H */

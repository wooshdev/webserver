/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef HTTP_HEADER_LIST_H
#define HTTP_HEADER_LIST_H

#include <stddef.h>

/* Some common used headers to increase indexing. */
typedef enum {
	/* ':authority' and 'host' */
	HEADER_AUTHORITY = 0x1,
	/* ":path"+"/" and ":path"+"/index.html" and request_line_part_two */
	HEADER_PATH = 0x2,
} http_defined_name_type;

typedef enum {
	/* key and value MUST NOT be freed. */
	HTTP_HEADER_CACHED = 0x0,
	/* only value should be freed (key != NULL & defined_name == 0) */
	HTTP_HEADER_NAME_CACHED = 0x1,
	/* only value should be freed (key == NULL & defined_name != 0) */
	HTTP_HEADER_NAME_DEFINED = 0x2,
	/* the key and value should both be freed. */
	HTTP_HEADER_NOT_CACHED = 0x3
} http_header_type;

typedef struct {
	http_header_type type;
	http_defined_name_type defined_name;
	const char *key;
	char *value;
} http_header_t;

typedef struct {
	http_header_t **headers;
	size_t count;
	/* Important: size != count */
	size_t size;
} http_header_list_t;

/** debugging purposes */
extern const char *http_header_type_names[];


/** functions */

/**
 * Description:
 *   Creates and sets up an empty header list.
 *
 * Return Value:
 *   See description.
 */
http_header_list_t *http_create_header_list();
void http_destroy_header_list(http_header_list_t *);

/**
 * Description:
 *   Get the value of a header using a key in defined-name form. This is faster, because integer-comparison is way faster than strint-based.
 *
 * Parameters:
 *   http_header_list_t *
 *     The header list.
 *   http_defined_name_type
 *     The header key.
 *
 * Return Value:
 *   A NULL-termimated header value, or NULL.
 */
const char *http_header_list_getd(http_header_list_t *, http_defined_name_type);

/**
 * Description:
 *   Get the value of a header using a key in string form.
 *
 * Parameters:
 *   http_header_list_t *
 *     The header list.
 *   const char *
 *     The header key.
 *
 * Return Value:
 *   A NULL-termimated header value, or NULL.
 */
const char *http_header_list_gets(http_header_list_t *, const char *);

/**
 * Description:
 *   Adds a header to the list.
 *
 * Parameters:
 *   http_header_list_t *
 *     The header list to add the header to.
 *   const char *
 *     The header key.
 *   char *
 *     The header value.
 *   http_header_type
 *     The type of the header. This specifies how and if the key & value should be freed.
 *   size_t
 *     The position in the static table. (The use of this variable is still incorrect.)
 *
 * Return Value:
 *   (Boolean) Success Status
 */
int http_header_list_add(http_header_list_t *, const char *, char *, http_header_type, size_t);

#endif /* HTTP_HEADER_LIST_H */

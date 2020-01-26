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
	size_t size;
} http_header_list_t;

/** debugging purposes */
extern const char *http_header_type_names[];


/** functions */
http_header_list_t *http_create_header_list();
void http_destroy_header_list(http_header_list_t *);
const char *http_header_list_getd(http_header_list_t *, http_defined_name_type);
const char *http_header_list_gets(http_header_list_t *, const char *);
int http_header_list_add(http_header_list_t *, const char *, char *, http_header_type, size_t);

#endif /* HTTP_HEADER_LIST_H */

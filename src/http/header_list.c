/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "header_list.h"

#include <stdlib.h>
#include <strings.h>
#include <stdio.h>

/**
 * To be tactical, I did a quick header-count test:
 * 
 * |----------------------------------------------|--------------|----------------|
 * |                  User Agent                  | Header Count | Browser Engine |
 * |----------------------------------------------|--------------|----------------|
 * |               Safari (iOS 13.3)              |       8      |     WebKit     |
 * |   Microsoft Internet Explorer (Trident/7.0)  |       8      |    Trident 7.0 |
 * |            Microsoft Edge/18.1776            |       9      |     EdgeHTML   |
 * |           Firefox Nightly (74.0a1)           |      10      |   Gecko/Necko  |
 * |         Tor 9.0.5 (Firefox 68.4.1esr)        |      10      |   Gecko/Necko  |
 * |         Microsoft Edg(e) 79.0.309.71         |      12      |    Chromium    |
 * |        Chrome & Chromium 79.0.3945.117       |      12      |    Chromium    |
 * | Brave Version 1.2.43 Chromium: 79.0.3945.130 |      13      |    Chromium    |
 * |----------------------------------------------|--------------|----------------|
 */
#define HEADER_LIST_STEP_SIZE    2
#define HEADER_LIST_INITIAL_SIZE 8

const char *http_header_type_names[] = { "HTTP_HEADER_CACHED", "HTTP_HEADER_NAME_CACHED", "HTTP_HEADER_NAME_DEFINED", "HTTP_HEADER_NOT_CACHED" };

http_header_list_t *http_create_header_list() {
	http_header_list_t *list = malloc(sizeof(http_header_list_t));
	if (!list) 
		return list;
	list->count = 0;
	list->size = HEADER_LIST_INITIAL_SIZE;
	list->headers = calloc(HEADER_LIST_INITIAL_SIZE, sizeof(http_header_t *));
	return list;
}

void http_destroy_header_list(http_header_list_t *list) {
	size_t i;
	for (i = 0; i < list->count; i++) {
		http_header_t *header = list->headers[i];
		switch (header->type) {
			case HTTP_HEADER_CACHED:
				/* key and value MUST NOT be freed. */
				break;
			case HTTP_HEADER_NOT_CACHED:
				/* the key and value should both be freed. */
				free((char *)header->key);
			case HTTP_HEADER_NAME_CACHED:
				/* only value should be freed (key != NULL & defined_name == 0) */
			case HTTP_HEADER_NAME_DEFINED:
				/* only value should be freed (key == NULL & defined_name != 0) */
				free(header->value);
				break;
			default:
				break;
		}
		free(header);
	}
	free(list->headers);
	free(list);
}

const char *http_header_list_getd(http_header_list_t *list, http_defined_name_type type) {
	size_t i;
	for (i = 0; i < list->count; i++) {
		if (list->headers[i]->type == HTTP_HEADER_NAME_DEFINED && list->headers[i]->defined_name == type) {
			return list->headers[i]->value;
		}
	}
	return NULL;
}

const char *http_header_list_gets(http_header_list_t *list, const char *key) {
	size_t i;
	for (i = 0; i < list->count; i++) {
		if (strcasecmp(list->headers[i]->key, key) == 0) {
			return list->headers[i]->value;
		}
	}
	
	return NULL;
}

int http_header_list_add(http_header_list_t *list, const char *key, char *value, http_header_type type, size_t static_table_position) {
	if (list->count+1 == list->size) {
		http_header_t **headers = realloc(list->headers, (list->size += HEADER_LIST_STEP_SIZE) * sizeof(http_header_t *));
		if (!headers) {
			return 0;
		}
		list->headers = headers;
	}
	
	http_header_t *header = calloc(1, sizeof(http_header_t));
	if (!header) {
		return 0;
	}
	
	header->type = type;
	switch (type) {
		case HTTP_HEADER_NAME_DEFINED: {
			if (static_table_position == 0) {
				if (strcasecmp(key, "server") == 0)
					header->defined_name = HEADER_AUTHORITY;
				else if (strcasecmp(key, ":path") == 0)
					header->defined_name = HEADER_AUTHORITY;
			} else {
				switch (static_table_position) {
					case 1:
						header->defined_name = HEADER_AUTHORITY;
						break;
					case 4:
					case 5:
						header->defined_name = HEADER_PATH;
						break;
					default:
						break;
				}
			}
			
		} /* no break here *purposely done* */
		default:
			header->key = key;
			header->value = value;
			break;
	}
	
	list->headers[list->count] = header;
	/*
	printf("HTTPHeaderList> Added=%p (count=%zu) key=%s value=%s\n", header, list->count, key, value);
	*/
	
	list->count += 1;
	
	return 1;
}

/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "util.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

int strswitch(const char *in, const char **list, size_t size, int case_flag) {
	if (!in)
		return -1;
	
	char *buffer = NULL;
	const char *input = in;
	
	size_t i;
	if (case_flag == CASEFLAG_IGNORE_B) {
		size_t lsize = strlen(in);
		
		buffer = malloc(lsize * sizeof(char));
		buffer[lsize] = 0;
		
		for (i = 0; i < lsize; i++)
			buffer[i] = tolower(in[i]);
		input = buffer;
	}
	
	for (i = 0; i < size; i++) {
		if ((case_flag == CASEFLAG_IGNORE_A && !strcasecmp(in, list[i])) || !strcmp(input, list[i])) {
			free(buffer);
			return i;
		}
	}
	
	free(buffer);
	return -1;
}

int strstartsw(const char *a, const char *b) {
	size_t lena = strlen(a),
		   lenb = strlen(b);
	return lena < lenb ? 0 : memcmp(a, b, lenb) == 0;
}

char *strdup(const char *src) {
	size_t length = strlen(src);
	char *copy = malloc((length+1) * sizeof(char));
	copy[length] = 0;
	if (!copy)
		return NULL;
	size_t i;
	for (i = 0; i < length; i++)
		copy[i] = src[i];
	return copy;
}

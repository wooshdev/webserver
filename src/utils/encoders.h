/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef ENCODERS_H
#define ENCODERS_H

#include <stddef.h>

typedef struct {
	char *data;
	size_t size;
} encoded_data_t;

encoded_data_t *encode_gzip(const char *data, size_t length);

#endif /* ENCODERS_H */
 

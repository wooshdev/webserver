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

/**
 * Description:
 *   Compress the message in the GZip format using zlib.
 * 
 * Parameters:
 *   const char *
 *     The data ("message") to compress.
 *   size_t
 *     The length of the message.
 * 
 * Notes:
 *   The return value 'encoded_data_t *' and its member 'char *data' should both be 
 *   freed, since these cannot and will not be cleaned by this function.
 * 
 * Return Value:
 *   The encoded_data_t structure with its data and size.
 */
encoded_data_t *encode_gzip(const char *, size_t );

#endif /* ENCODERS_H */
 

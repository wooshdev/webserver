/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "encoders.h"

#include <stdio.h>
#include <stdlib.h>

#include <zlib.h>

#define CALL_ZLIB(x) {\
		int status;\
		status = x;\
		if (status < 0) {\
			fprintf (stderr, "%s:%d: %s returned a bad status of %d.\n", __FILE__, __LINE__, #x, status); \
			exit(EXIT_FAILURE);\
		}\
	}

#define windowBits 15
#define GZIP_ENCODING 16

encoded_data_t *encode_gzip(const char *input, size_t length) {
	/* Files can be large in size, so maybe use this functions in 
	 * chunked phases, so now enormous blobs have to be allocated. */
	unsigned char *out = malloc(sizeof(char) * length);
	if (!out)
		return NULL;
	
	encoded_data_t *result = malloc(sizeof(encoded_data_t));
	if (!result) {
		free(out);
		return NULL;
	}
	
	
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree  = Z_NULL;
	strm.opaque = Z_NULL;
    CALL_ZLIB(deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
							windowBits | GZIP_ENCODING, 8,
							Z_DEFAULT_STRATEGY));
	strm.next_in = (unsigned char *) input;
	strm.avail_in = length;
	
	do {
		strm.avail_out = length;
		strm.next_out = out + strm.total_out;
		CALL_ZLIB(deflate(&strm, Z_FINISH));
	} while (strm.avail_out == 0);
	deflateEnd(&strm);
	
	result->data = (char *) out;
	result->size = strm.total_out;
	
	return result;
}

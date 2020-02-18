/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "encoders.h"

#include <stdio.h>
#include <stdlib.h>

#include <zlib.h>

#ifdef ENCODERS_ENABLE_BROTLI
	#include <brotli/encode.h>
#endif /* ENCODERS_ENABLE_BROTLI */

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

/** Section: Variables */
int ENCODER_STATUS_gzip,
	ENCODER_STATUS_brotli;
int initialized = 0;
#ifdef ENCODERS_ENABLE_BROTLI
	BrotliEncoderState *brotli_state;
	int brotli_quality, brotli_window;
	BrotliEncoderMode brotli_mode;
#endif /* ENCODERS_ENABLE_BROTLI */

int encoder_setup(void) {
	if (initialized)
		return 1;

	#ifdef ENCODERS_ENABLE_BROTLI
		/* BrotliEncoderState *BrotliEncoderCreateInstance(brotli_alloc_func alloc_func, brotli_free_func free_func, void *opaque) */
		if ((brotli_state = BrotliEncoderCreateInstance(NULL, NULL, NULL)) == NULL) {
			puts("[Encoder] Failed to initialize brotli!");
			return 0;
		}

		brotli_quality = BROTLI_DEFAULT_QUALITY;
		brotli_window = BROTLI_DEFAULT_WINDOW;
		brotli_mode = BROTLI_DEFAULT_MODE;
		ENCODER_STATUS_brotli = 1;
	#else
		ENCODER_STATUS_brotli = 0;
	#endif /* ENCODERS_ENABLE_BROTLI */
	ENCODER_STATUS_gzip = 1;

	initialized = 1;
	return 1;
}

void encoder_destroy(void) {
	if (initialized) {
		#ifdef ENCODERS_ENABLE_BROTLI
			BrotliEncoderDestroyInstance(brotli_state);
			brotli_state = NULL;
		#endif /* ENCODERS_ENABLE_BROTLI */
		initialized = 0;
	}
}

encoded_data_t *encode_brotli(const char *input, size_t length) {
#ifdef ENCODERS_ENABLE_BROTLI
	encoded_data_t *result = malloc(sizeof(encoded_data_t));
	if (!result) {
		printf("[Encoder] (Brotli) Mallocation error [encoded_data]: size=%zu\n", result->size);
		return NULL;
	}

	result->size = BrotliEncoderMaxCompressedSize(length);
	if (result->size == 0) {
		printf("[Encoder] (Brotli) BrotliEncoderMaxCompressedSize failed: length is too big: %zu\n", length);
		free(result);
		return NULL;
	}

	result->data = malloc(result->size);
	if (!result->data) {
		printf("[Encoder] (Brotli) Mallocation error [char[] output]: size=%zu\n", result->size);
		free(result);
		return NULL;
	}

	if (BrotliEncoderCompress(brotli_quality, brotli_window, brotli_mode, length, (const uint8_t *)input, &result->size, (uint8_t *)result->data) == BROTLI_FALSE) {
		printf("[Encoder] (Brotli) Compression error: size=%zu\n", result->size);
		free(result->data);
		free(result);
		return NULL;
	}

	return result;
#else
	return NULL;
#endif /* ENCODERS_ENABLE_BROTLI */
}

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
		/* I do this in a do-while loop since someone else did it (facepalm) but I haven't tested this code very good. */
		strm.avail_out = length;
		strm.next_out = out + strm.total_out;
		CALL_ZLIB(deflate(&strm, Z_FINISH));
	} while (strm.avail_out == 0);
	deflateEnd(&strm);
	
	result->data = (char *) out;
	result->size = strm.total_out;
	
	return result;
}

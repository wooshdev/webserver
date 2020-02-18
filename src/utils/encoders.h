/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef ENCODERS_H
#define ENCODERS_H

#include <stddef.h>

/** Is GZIP available? */
extern int ENCODER_STATUS_gzip;

/** Is brotli available? */
extern int ENCODER_STATUS_brotli;

/** Note: to use brotli, add a macro called ENCODERS_ENABLE_BROTLI */

typedef struct {
	char *data;
	size_t size;
} encoded_data_t;

/**
 * Description:
 *   This is the setup function for the encoder header. You shouldn't use the encoder 
 *	 header's function, except for 'encoder_setup' before calling 'encoder_setup'.
 *   Calling this function twice without calling 'encoder_destroy' has no side effects.
 * 
 * Return Value:
 *   (boolean) Success Status
 */
int encoder_setup(void);

/**
 * Description:
 *	 This function will destroy all data create & allocated by encoder_destroy. Calling 
 *	 this function twice without calling 'encoder_setup' again has no side effects.
 * 
 * Return Value:
 *   (boolean) Success Status
 */
void encoder_destroy(void);

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

/**
 * Description:
 *   Compress the message in the brotli format using Google's Library.
 *
 * Warning(s):
 *   This function will ALWAYS return NULL if 'ENCODER_STATUS_brotli' is 0.
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
encoded_data_t *encode_brotli(const char *, size_t);

#endif /* ENCODERS_H */
 

/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains function declarations for reading a HTTP/2 frame.
 */
#ifndef HTTP2_FRAME_H
#define HTTP2_FRAME_H
#include <stdint.h>

/** For the 'TLS' typedef. */
#include "../secure/tlsutil.h"

typedef struct {
	unsigned int length : 24;
	unsigned char type;
	unsigned char flags;
	uint32_t r_s_id;/* r + stream identifier */
	char *data;
} frame_t;

/**
 * Description:
 *   Reads a frame.
 * 
 * Parameters:
 *   TLS
 *     The source to read from.
 * 
 * Return Value:
 *   A 'frame *', or NULL if failed.
 */
frame_t *readfr(TLS);

/**
 * Description:
 *   Sends a frame.
 * 
 * Parameters:
 *   Except for 'TLS' as the source, are all the parameters types of HTTP frames.
 *   See RFC 7540 Section 4.1
 * 
 * Return Value:
 *   (boolean) I/O success status 
 */
int send_frame(TLS, uint32_t length, char type, char flags, uint32_t stream, char *data);

#endif /* HTTP2_FRAME_H */

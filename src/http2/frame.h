/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains function declarations for reading a HTTP/2 frame.
 */
#ifndef HTTP2_FRAME_H
#define HTTP2_FRAME_H
#include <stdint.h>

#define FRAME_DATA          0x0
#define FRAME_HEADERS       0x1
#define FRAME_PRIORITY      0x2
#define FRAME_RST_STREAM    0x3
#define FRAME_SETTINGS      0x4
#define FRAME_PUSH_PROMISE  0x5
#define FRAME_PING          0x6
#define FRAME_GOAWAY        0x7
#define FRAME_WINDOW_UPDATE 0x8
#define FRAME_CONTINUATION  0x9
#define FRAME_ALTSVC        0xa
#define FRAME_ORIGIN        0xc

/** For the 'TLS' typedef. */
#include "../secure/tlsutil.h"

/* For the H2_ERROR enum */
#include "constants.h"

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
 *   uint32_t
 *     The maximum size for one frame.
 *   H2_ERROR *
 *     The pointer to an address where 
 *     the error code is set, if an error 
 *     has occurred (i.e. if the return 
 *     value is NULL).
 * 
 * Return Value:
 *   A 'frame *', or NULL if failed.
 */
frame_t *readfr(TLS, uint32_t, H2_ERROR *);

/**
 * Description:
 *   Sends a frame.
 * 
 * Parameters:
 *   Except for 'TLS' as the source and max_size, are all the parameters types of HTTP frames.
 *   See RFC 7540 Section 4.1
 * 
 * Return Value:
 *   (boolean) I/O success status 
 */
int send_frame(TLS, /*uint32_t max_size, */uint32_t length, char type, char flags, uint32_t stream, const char *data);

#endif /* HTTP2_FRAME_H */

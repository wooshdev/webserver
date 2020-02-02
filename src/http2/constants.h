/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains symbols defined by RFC 7540 and RFC 7541. These are unlikely 
 * to be changed by W3C and shouldn't be changed by the application.
 */
#ifndef HTTP2_CONSTANTS_H
#define HTTP2_CONSTANTS_H
#include <stddef.h>	/* for NULL */
#include <stdint.h> /* for uint32_t */

/* Ff the amount of settings gets changed, change this value */
#define HTTP2_SETTINGS_COUNT 6

/*SETTINGS_HEADER_TABLE_SIZE", "SETTINGS_ENABLE_PUSH", "SETTINGS_MAX_CONCURRENT_STREAMS", "SETTINGS_INITIAL_WINDOW_SIZE", "SETTINGS_MAX_FRAME_SIZE", "SETTINGS_MAX_HEADER_LIST_SIZE*/
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE 0x4
#define HTTP2_SETTINGS_TABLE_SIZE 0x1

typedef enum {
	H2_NO_ERROR = 0x0,
	H2_PROTOCOL_ERROR = 0x1,
	H2_INTERNAL_ERROR = 0x2,
	H2_FLOW_CONTROL_ERROR = 0x3,
	H2_SETTINGS_TIMEOUT = 0x4,
	H2_STREAM_CLOSED = 0x5,
	H2_FRAME_SIZE_ERROR = 0x6,
	H2_REFUSED_STREAM = 0x7,
	H2_CANCEL = 0x8,
	H2_COMPRESSION_ERROR = 0x9,
	H2_CONNECT_ERROR = 0xA,
	H2_ENHANCE_YOUR_CALM = 0xB,
	H2_INADEQUATE_SECURITY = 0xC,
	H2_HTTP_1_1_REQUIRED = 0xD
} H2_ERROR;

#define FLAG_ACK 0x1
#define FLAG_END_STREAM 0x1
#define FLAG_END_HEADERS 0x4
#define FLAG_PADDED 0x8
#define FLAG_PRIORITY 0x20 

#define PRTERR(text) fprintf(stderr, "\x1b[31m%s\x1b[0m", text)
#define BITS31 0xEFFFFFFF


/* RFC 7540: Section 6.5.2 */
extern const char *settings_ids[];
/* RFC 7540: Section 6.x */
extern const char *frame_types[];
/* RFC 7540: Section 7 */
extern const char *h2_error_codes[];

/* RFC 7540: Section 3.5 */
extern const char *preface;

/**
 * Description:
 *   Convert four octets to a uint32_t.
 */
uint32_t u32(const char *);

#endif /* HTTP2_CONSTANTS_H */

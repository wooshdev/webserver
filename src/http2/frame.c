/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains function definitions for reading a HTTP/2 frame.
 */
#include "frame.h"
#include <stdlib.h>
#include <string.h>
#include "constants.h"

frame_t *readfr(TLS tls, uint32_t max_size, H2_ERROR *error) {
	frame_t *f = calloc(1, sizeof(frame_t));
	if (!f)
		return NULL;
	char *parts = calloc(4, sizeof(char));

	if (!tls_read_client_complete(tls, parts, 3)) {
		free(f);
		free(parts);
		return NULL;
	}
	
	f->length = ((parts[0] & 0xFF) << 16) | ((parts[1] & 0xFF)  << 8) | (parts[2] & 0xFF);
	
	if (f->length > max_size) {
		printf("\x1b[36mFrame> \x1b[33mMaxSizeExceedError: max=%u length=%u\n", max_size, f->length);
		*error = H2_FRAME_SIZE_ERROR;
		/*free(f);
		return NULL;*/
	}
	
	if (!tls_read_client_complete(tls, (char *)&f->type, sizeof(f->type))) {
		free(f);
		free(parts);
		return NULL;
	}
	
	if (f->length > max_size) {
		printf("\x1b[36mFrame> type=%s (0x%x) (Error)\x1b[0m\n", frame_types[f->type], f->type);
		free(f);
		free(parts);
		return NULL;
	}
	
	if (!tls_read_client_complete(tls, (char *)&f->flags, sizeof(f->flags))) {
		free(f);
		free(parts);
		return NULL;
	}
	
	if (!tls_read_client_complete(tls, parts, 4)) {
		free(f);
		free(parts);
		return NULL;
	}
	f->r_s_id = u32(parts);
	
	if (!(f->data = malloc(sizeof(char) * f->length))) {
		free(f);
		free(parts);
		return NULL;
	}
	
	if (f->length && !tls_read_client_complete(tls, f->data, f->length)) {
		free(f->data);
		free(f);
		free(parts);
		return NULL;
	}
	
	#ifdef FRAME_READ_ANNOUNCE
	printf("\x1b[36mFrame> type=%s (0x%x) stream=%x length=%u\x1b[0m\n", frame_types[f->type], f->type, f->r_s_id, f->length);
	#endif
	
	free(parts);
	return f;
}

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
/*#define FRAME_SEND_DEBUG*/
int send_frame(TLS tls, uint32_t length, char type, char flags, uint32_t stream, const char *data) {
	#ifdef FRAME_SEND_DEBUG_VERBOSE
	printf("[\x1b[33mSendFrame\x1b[0m] \x1b[33mType: %s Stream: 0x%x\x1b[0m\n", frame_types[(size_t)type], stream);
	#endif
  
	#ifdef FRAME_SEND_DEBUG_VERBOSE
	printf("\x1b[33m`-> length=%u type=%hi flags=0x%hx stream=%u pdata=%p\n\x1b[0m", length, type, flags, stream, data);
	#endif
  
	char *buf = malloc(9 + length);
	if (!buf) return 0;
	buf[0] = length >> 16;
	buf[1] = length >> 8;
	buf[2] = length & 0x000000FF;
	buf[3] = type;
	buf[4] = flags;
	buf[5] = stream >> 24;
	buf[6] = stream >> 16;
	buf[7] = stream >> 8;
	buf[8] = stream & 0x000000FF;
	
	#ifdef FRAME_SEND_DEBUG_VERBOSE
	printf("%i %i\n", !!(data), !!(length));
	#endif
  
	if (data && length) {
		memcpy(buf+9, data, length);
  }
  
	#ifdef FRAME_SEND_DEBUG_VERBOSE
	size_t j;
	for (j = 0; j < 9 + length; j++)
		printf("\x1b[33m> (%zu) 0x%hhx\n\x1b[0m", j, buf[j]);
	#endif
  
	int res = tls_write_client(tls, buf, 9 + length);
	free(buf);
	return res;
}
 

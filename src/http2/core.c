/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains the main functions for HTTP/2.
 */
#include "core.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "../http/parser.h"
#include "../utils/io.h"
#include "../utils/util.h"
#include "../http/header_list.h"
#include "../http/response_headers.h"

#include "constants.h"
#include "dynamic_table.h"
#include "frame.h"
#include "hpack.h"
#include "huffman.h"
#include "stream.h"

/* string compare with length */
int scomp(const char *a, const char *b, size_t len) {
	size_t i;
	for (i = 0; i < len; i++)
		if (a[i] != b[i])
			return 0;
	return 1;
}

static const char *settings_names[] = { NULL, "SETTINGS_HEADER_TABLE_SIZE", "SETTINGS_ENABLE_PUSH", "SETTINGS_MAX_CONCURRENT_STREAMS", "SETTINGS_INITIAL_WINDOW_SIZE", "SETTINGS_MAX_FRAME_SIZE", "SETTINGS_MAX_HEADER_LIST_SIZE" };

static H2_ERROR handle_settings(frame_t *frame, setentry_t *settings) {
	setentry_t ent;
  printf("[\x1b[33mSettings\x1b[0m] \x1b[32mSize: %u Flags: 0x%x stream_id: 0x%x\n", frame->length, frame->flags, frame->r_s_id & 0xEF);
	if (frame->length > 0) {
		size_t i;
		for (i = 0; i < frame->length / 6; i++) {
			char *start = frame->data + i*6;
			ent.id = (start[0] << 8) | start[1];
			ent.value = u32(start+2);
			
			if (ent.id >= HTTP2_SETTINGS_COUNT) {
				fprintf(stderr, "[H2] Invalid setting identifier: %hu with value %u\n", ent.id, ent.value);
				return H2_PROTOCOL_ERROR;
			}
			
			#ifdef _NOT_DEFINED
			uint32_t max = 2147483647;
			if (ent.id == HTTP2_SETTINGS_INITIAL_WINDOW_SIZE && ent.value > max) {
				fprintf(stderr, "[H2] Invalid value for initial window size: %u, max: %u\n", ent.value, max);
				return H2_FLOW_CONTROL_ERROR;
			}
			#endif
			
			printf("[\x1b[33mSettings\x1b[0m] \x1b[32mName: %s Value: %u\n", settings_names[ent.id], ent.value);
			
			settings[ent.id-1].value = ent.value;
		}
	}
	
	return H2_NO_ERROR;
}

/**
 * Description:
 *   Sends the settings to the client.
 *
 * Return Value:
 *   (boolean) success status
 */
static int send_settings(TLS tls) {
	char *buf = malloc(6);
	buf[0] = 0x00;
	buf[1] = 0x04; /* SETTINGS_INITIAL_WINDOW_SIZE */
	buf[2] = 0x00;
	buf[3] = 0xBF;
	buf[4] = 0x00;
	buf[5] = 0x01;
	
	return send_frame(tls, 6, FRAME_SETTINGS, 0x0, 0x0, buf);
}

static int send_settings_ack(TLS tls) {
	return send_frame(tls, 0, FRAME_SETTINGS, FLAG_ACK, 0x0, NULL);
}

static void send_rst(TLS tls, uint32_t error) {
	printf("[\x1b[33m[!] Sending RST_STREAM frame! Error: %s\x1b[0m\n", h2_error_codes[error]);
	send_frame(tls, 4, FRAME_RST_STREAM, 0, 0, (char *)&error);
}

static void send_goaway(TLS tls, uint32_t error, uint32_t stream) {
	printf("[\x1b[33m[!] Sending GOWAY frame! Error: %s\x1b[0m\n", h2_error_codes[error]);
	char *buf = malloc(8);
	buf[0] = (stream >> 24) & 0xFF;
	buf[1] = (stream >> 16) & 0xFF;
	buf[2] = (stream >> 8) & 0xFF;
	buf[3] = stream & 0xFF;
	buf[4] = (error >> 24) & 0xFF;
	buf[5] = (error >> 16) & 0xFF;
	buf[6] = (error >> 8) & 0xFF;
	buf[7] = error & 0xFF;
	send_frame(tls, 8, FRAME_GOAWAY, 0x0, 0x0, buf);
	free(buf);
}

static void write_str(char *data, const char *str, size_t *pos) {
	size_t j, length = strlen(str);
	printf("String length: 0x%zx or %zu\n", length, length);
	data[(*pos)] = length;
	
	(*pos) += 1;
	for (j = 0; j < length; j++) {
		data[(*pos) + j] = str[j];
	}
	(*pos) += j;
}

static const char *simple = "<body style=\"color:white;background:black;display:flex;align-items:center;width:100%;height:100%;justify-items:center;font-family:-apple-system,BlinkMacSystemFont,sans-serif;font-size:60px;text-align:center\"><h1>This is sent via HTTP/2!</h1></body>";

static void handle_simple(TLS tls, frame_t *frame) {
	/* headers MUST not get strings longer than 128 octets in size. */
	char *length_buffer = calloc(128, sizeof(char));
	sprintf(length_buffer, "%zu", strlen(simple));
	
	http_response_headers_t *list = http_create_response_headers(3);
	http_response_headers_add(list, HTTP_RH_STATUS_200, NULL);
	http_response_headers_add(list, HTTP_RH_CONTENT_LENGTH, length_buffer);
	http_response_headers_add(list, HTTP_RH_CONTENT_TYPE, "text/html; charset=UTF-8");
	http_response_headers_add(list, HTTP_RH_SERVER, "TheWooshServer");
	
	char *headers = calloc(1, 256);
	size_t i, pos = 0;
	http_response_header_t *header;
	printf(" Header count: %zu\n", list->count);
	for (i = 0; i < list->count; i++) {
		/*headers[pos] &= (1 << i);*/
		header = list->headers[i];
		switch (header->name) {
			case HTTP_RH_CONTENT_LENGTH:
				/* 0x5C = 01011100 */
				headers[pos] = 0x5C;
				pos += 1;
				write_str(headers, header->value, &pos);
				break;
			case HTTP_RH_CONTENT_TYPE:
				/* 0x5F = 01011111 */
				headers[pos] = 0x5F;
				pos += 1;
				write_str(headers, header->value, &pos);
				break;
			case HTTP_RH_SERVER:
				/* 0x76 = 01110110 */
				/*put_bits(headers, 0x76, pos);*/
				headers[pos] = 0x76;
				pos += 1;
				write_str(headers, header->value, &pos);
				break;
			case HTTP_RH_STATUS_200:
				/* 0x88 = 10001000 */
				headers[pos] = 0x88;
				pos += 1;
				break;
			default:
				printf("(?) Unknown header type: %u\n", header->name);
				break;
		}
	}
	/* Add EOS padding */
/* 	size_t bpleft = bp % 8;
 	if (bpleft != 0) {
 		for (i = 0; i < bpleft; i++) {
 			headers[bp/8] &= (1 << i);
 		}
 		bp += bpleft;
 	}
 	*/
	free(length_buffer);
	http_response_headers_destroy(list);

	/* reparse to see if/where the (an) error is. */ /*{
		frame_t *temp_frame = malloc(sizeof(frame_t));
		temp_frame->flags = FLAG_END_HEADERS;
		temp_frame->length = pos;
		temp_frame->data = headers;
		http_header_list_t *temp_headers = http_create_header_list();
		dynamic_table_t *dynamic_table = dynamic_table_create(4096);
		handle_headers(temp_frame, dynamic_table, temp_headers);
		dynamic_table_destroy(dynamic_table);
		http_destroy_header_list(temp_headers);
		free(temp_frame);
	}*/
	
	printf(" > sending HEADERS frame, pos=%zu\n", pos);
	send_frame(tls, pos, FRAME_HEADERS, FLAG_END_HEADERS, frame->r_s_id, headers);
	free(headers);
	
	printf(" > sending DATA frame, strlen=%zu\n", strlen(simple));
	send_frame(tls, strlen(simple), FRAME_DATA, FLAG_END_STREAM, frame->r_s_id, simple);
}

http_request_t http2_parse(TLS tls) {
	http_request_t req = { 0 };
	
	uint32_t window_size = UINT16_MAX;
	size_t settings_count = HTTP2_SETTINGS_COUNT;
	setentry_t *settings = calloc(settings_count, sizeof(setentry_t));
	/* default values as per 6.5.2 */
	settings[0].id = 0x1;           /* SETTINGS_HEADER_TABLE_SIZE */
	settings[0].value = 4096;       /* 2^12 */
	settings[1].id = 0x2;           /* SETTINGS_ENABLE_PUSH */
	settings[1].value = 1;          /* initial value: 1 (true) */
	settings[2].id = 0x3;           /* SETTINGS_MAX_CONCURRENT_STREAMS */
	settings[2].value = 100;        /* recommended lower limit: 100 */
	settings[3].id = 0x4;           /* SETTINGS_INITIAL_WINDOW_SIZE */
	settings[3].value = 65535;      /* initial value: 2^16-1 */
	settings[4].id = 0x5;           /* SETTINGS_MAX_FRAME_SIZE */
	settings[4].value = 16384;      /* initial value: 2^14 */
	settings[5].id = 0x5;           /* SETTINGS_MAX_HEADER_LIST_SIZE */
	settings[5].value = UINT32_MAX; /* initial value: unset */
	
	puts("\x1b[32mbegin\x1b[0m");
	
	char prefacebuf[24] = { 0 };
	if (!tls_read_client_complete(tls, prefacebuf, 24) || !scomp(prefacebuf, preface, 24)) {
		PRTERR("[H2] Preface io/comparison failure.\n");
		return req;
	}
	H2_ERROR error = H2_NO_ERROR;
	
	frame_t *frame = readfr(tls, settings[4].value, &error);
	
	/** Dynamic Table */
	dynamic_table_t *dynamic_table = NULL;
	http_header_list_t *headers = NULL;
	size_t i;
	
	send_settings(tls);
	/* send WINDOW_UPDATE frame */{
		uint32_t size = 0x7FFF0000;
		send_frame(tls, 4, FRAME_WINDOW_UPDATE, 0x0, 0x0, (char *)&size);
	}
	
	if (frame) {
		if (frame->type != 0x4) {
			PRTERR("[H2] Protocol error: first frame wasn't a settings frame!");
			send_goaway(tls, H2_PROTOCOL_ERROR, 0x0);
			goto end;
		}
    
		/* SETTINGS frames should have a length of a multiple of 6 octets. */
		if (frame->length % 6 != 0) {
			puts("\x1b[33m > Invalid settings frame (length error).\x1b[0m");
			send_goaway(tls, H2_FRAME_SIZE_ERROR, 0x0);
			goto end;
		}
	
		H2_ERROR result = handle_settings(frame, settings);
		free(frame->data);
		free(frame);
    
		if (result != H2_NO_ERROR) {
			puts("\x1b[33m > Invalid settings frame.\x1b[0m");
			send_goaway(tls, H2_FRAME_SIZE_ERROR, 0x0);
			goto end;
		}
		
		send_settings_ack(tls);
		headers = http_create_header_list();
		
		size_t previous_type = 0x4;
		
		while ((frame = readfr(tls, settings[4].value, &error))) {
			switch (frame->type) {
				case FRAME_DATA:
					
					break;
				case FRAME_HEADERS:
					if (!dynamic_table) {
						size_t client_max_size = 0;
						for (i = 0; i < settings_count; i++) {
							if (settings[i].id == HTTP2_SETTINGS_TABLE_SIZE) {
								client_max_size = settings[i].value;
							}
						}
						dynamic_table = dynamic_table_create(client_max_size);
					}
					
					/*handle_headers(frame, dynamic_table, headers);*/
					
					if (frame->flags & FLAG_END_HEADERS) {
						/*puts("+======== HeaderList ========+");
						printf("Count: %zu size: %zu ptr=%p ptrparent=%p\n", headers->count, headers->size, headers->headers, headers);
						for (i = 0; i < headers->count; i++) {
							printf(" > (%zu) Ptr=%p ", i, headers->headers[i]);
							printf("Key='%s' ", headers->headers[i]->key);
							printf("Value='%s' ", headers->headers[i]->value);
							printf("Type='%s'\n", http_header_type_names[headers->headers[i]->type]);
						}
						*/
						
						handle_simple(tls, frame);
						http_destroy_header_list(headers);
						headers = http_create_header_list();
					}
					break;
				case FRAME_PRIORITY: {
					if (frame->length != 5) {
						/* connection error */
						send_rst(tls, H2_FRAME_SIZE_ERROR);
						goto end;
					}
					
					char e = frame->data[0] & 0x10;
					uint32_t stream_dependency = u32(frame->data) & BITS31;
					uint8_t weight = frame->data[4];
					
					printf("\x1b[35m > Priority stream=%u e=%s s-depend=%u weight=%hhi\n", frame->r_s_id & BITS31, e ? "true" : "false", stream_dependency, weight);
				} break;
				case FRAME_RST_STREAM:
					fputs("\x1b[33mEnd (semi-gracefully) requested, ", stdout);
					if (frame->length == 4) {
						printf("reason: %s\x1b[0m\n", h2_error_codes[u32(frame->data)]);
					} else {
						puts("but the reason was corrupted.");
					}
					send_goaway(tls, H2_CANCEL, 0x0);
					goto end;
					break;
				case FRAME_SETTINGS:
					if (!(frame->flags & FLAG_ACK)) {
						send_settings_ack(tls);
					}
					break;
				case FRAME_PING:
					/* PING frames should have a length of 8 octets. */
					if (frame->length != 8) {
						puts("\x1b[33m > Invalid ping frame (length error).\x1b[0m");
						send_goaway(tls, H2_FRAME_SIZE_ERROR, 0x0);
						goto end;
					}
					if (!(frame->flags & FLAG_ACK)) {
						send_frame(tls, 8, FRAME_PING, FLAG_ACK, 0x0, frame->data);
					}
					break;
				case FRAME_GOAWAY:
					printf("\x1b[31m > GOAWAY ErrorCode=%s\x1b[0m\n", h2_error_codes[u32(frame->data+4)]);
					break;
				case FRAME_WINDOW_UPDATE:
					if (frame->length != 4) {
						/* connection error */
						send_rst(tls, H2_FRAME_SIZE_ERROR);
						goto end;
					}
					uint32_t wsi = u32(frame->data) & 0xEFFFFFFF;
					if (window_size == wsi) {
						/* connection error */
						puts("illegal");
						send_rst(tls, H2_PROTOCOL_ERROR);
						goto end;
					}
					printf(" > Size Increment: 0x%X or 0x%X = %u %u\n", u32(frame->data), wsi, u32(frame->data), wsi);
					printf(" > Additional data: a=0x%hhx b=0x%hhx c=0x%hhx d=0x%hhx\n", frame->data[0], frame->data[1], frame->data[2], frame->data[3]);
					break;
				case FRAME_CONTINUATION:
					switch (previous_type) {
						case 0x4: /* HEADERS */
							printf("Additional header block.. (not handled) = CONTINUATION\n");
							break;
						default:
							printf("CONTINUATION previous=0x%zx\n", previous_type);
							break;
					}
					break;
				default:
					puts("\x1b[31m > Type unknown (the frame is ignored without any consequence(s)).\x1b[0m");
					break;
			}
			
			free(frame->data);
			previous_type = frame->type;
			free(frame);
		}
		
		if (error != H2_NO_ERROR) {
			send_goaway(tls, error, 0x0);
		}
		
		printf("\x1b[32mEnd of frame stream. Reason: %s\n\x1b[0m\n", h2_error_codes[error]);
	} else {
		if (error == H2_NO_ERROR) {
			PRTERR("I/O failure for settings frame.");
		} else {
			send_goaway(tls, error, 0x0);
		}
	}
	
	end:
	if (dynamic_table)
		dynamic_table_destroy(dynamic_table);
	if (headers)
		http_destroy_header_list(headers);
	puts("\x1b[32mend\x1b[0m");
	return req;
}

int http2_setup() {
	return huff_setup();
}

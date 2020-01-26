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

#include "constants.h"
#include "dynamic_table.h"
#include "frame.h"
#include "hpack.h"
#include "huffman.h"

/* string compare with length */
int scomp(const char *a, const char *b, size_t len) {
	size_t i;
	for (i = 0; i < len; i++)
		if (a[i] != b[i])
			return 0;
	return 1;
}

static const char *settings_names[] = { NULL, "SETTINGS_HEADER_TABLE_SIZE", "SETTINGS_ENABLE_PUSH", "SETTINGS_MAX_CONCURRENT_STREAMS", "SETTINGS_INITIAL_WINDOW_SIZE", "SETTINGS_MAX_FRAME_SIZE", "SETTINGS_MAX_HEADER_LIST_SIZE" };

static int handle_settings(frame_t *frame, setentry_t *settings) {
	setentry_t ent;
	if (frame->length > 0) {
		size_t i;
		for (i = 0; i < frame->length / 6; i++) {
			char *start = frame->data + i*6;
			ent.id = (start[0] << 8) | start[1];
			ent.value = u32(start+2);
			
			if (ent.id >= HTTP2_SETTINGS_COUNT) {
				fprintf(stderr, "[H2] Invalid setting identifier: %hu with value %u\n", ent.id, ent.value);
				return 0;
			}
			
			printf("[\x1b[33mSettings\x1b[0m] \x1b[32mName: %s Value: %u\n", settings_names[ent.id], ent.value);
			
			settings[ent.id-1].value = ent.value;
		}
	}
	
	return 1;
}

/**
 * Description:
 *   Sends the settings to the client.
 *
 * Return Value:
 *   (boolean) success status
 */
static int send_settings(TLS tls) {
	/* for now we just acknowledge the sent settings */
	return send_frame(tls, 0, FRAME_SETTINGS, 0x1, 0, NULL);
}

static void send_rst(TLS tls, uint32_t error) {
	PRTERR("[!] Sending RST_STREAM frame!");
	send_frame(tls, 4, FRAME_RST_STREAM, 0, 0, (char *)&error);
}

static const char *simple = "Hello there!";

static void handle_simple(TLS tls, frame_t *frame) {
	http_header_list_t *list = http_create_header_list();
	http_header_list_add(list, ":status", strdup("200"), 8, HTTP_HEADER_CACHED);
	http_header_list_add(list, ":content-length", strdup("12"), 28, HTTP_HEADER_CACHED);
	http_header_list_add(list, "server", strdup("TheWooshServer"), 74, HTTP_HEADER_CACHED);
	
	char *headers = calloc(1, 256);
	size_t i, bp = 0;
	for (i = 0; i < list->count; i++) {
		/*headers[bp / 8] &= (1 << i);*/
	}
	size_t bpleft = bp % 8;
	if (bpleft != 0) {
		/* Add EOS padding */
		for (i = 0; i < bpleft; i++)
			headers[bp/8] &= (1 << i);
		bp += bpleft;
	}
	http_destroy_header_list(list);
	
	send_frame(tls, bp / 8, FRAME_HEADERS, FLAG_END_HEADERS, frame->r_s_id, headers);
	free(headers);
	
	char *data = strdup(simple);
	send_frame(tls, strlen(simple), FRAME_DATA, 0, frame->r_s_id, data);
	free(data);
}

http_request_t http2_parse(TLS tls) {
	http_request_t req = { 0 };
	
	uint32_t window_size = UINT16_MAX;
	size_t settings_count = HTTP2_SETTINGS_COUNT;
	setentry_t *settings = calloc(settings_count, sizeof(setentry_t));
	/* default values as per 6.5.2 */
	settings[0].id = 0x1;
	settings[0].value = 4096;
	settings[1].id = 0x2;
	settings[1].value = 1;
	settings[2].id = 0x3;
	settings[2].value = 100;
	settings[3].id = 0x4;
	settings[3].value = 65535;
	settings[4].id = 0x5;
	settings[4].value = 16384;
	settings[5].id = 0x5;
	settings[5].value = UINT32_MAX;
	
	puts("\x1b[32mbegin\x1b[0m");
	
	char prefacebuf[24] = { 0 };
	if (!tls_read_client_complete(tls, prefacebuf, 24) || !scomp(prefacebuf, preface, 24)) {
		PRTERR("[H2] Preface io/comparison failure.\n");
		return req;
	}
	
	frame_t *frame = readfr(tls);
	
	/** Dynamic Table */
	dynamic_table_t *dynamic_table = NULL;
	http_header_list_t *headers = NULL;
	size_t i;
	
	if (settings) {
		if (frame->type != 0x4) {
			PRTERR("[H2] Protocol error: first frame wasn't a settings frame!");
			goto end;
		}
		
		handle_settings(frame, settings);
		free(frame->data);
		free(frame);
		
		send_settings(tls);
		
		headers = http_create_header_list();
		
		size_t previous_type = 0x4;
		
		while ((frame = readfr(tls))) {
			switch (frame->type) {
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
					
					handle_headers(frame, dynamic_table, headers);
					
					if (frame->flags & FLAG_END_HEADERS) {
						puts("+======== HeaderList ========+");
						printf("Count: %zu size: %zu ptr=%p ptrparent=%p\n", headers->count, headers->size, headers->headers, headers);
						for (i = 0; i < headers->count; i++) {
							printf(" > (%zu) Ptr=%p ", i, headers->headers[i]);
							printf("Key='%s' ", headers->headers[i]->key);
							printf("Value='%s' ", headers->headers[i]->value);
							printf("Type='%s'\n", http_header_type_names[headers->headers[i]->type]);
						}
						
						
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
						printf("reason: %s\x1b[0m\n", error_codes[u32(frame->data)]);
					} else {
						puts("but the reason was corrupted.");
					}
					goto end;
					break;
				case FRAME_GOAWAY:
					printf("\x1b[31m > GOAWAY ErrorCode=%s\x1b[0m\n", error_codes[u32(frame->data+4)]);
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
					puts("\x1b[31m > Type unknown.\x1b[0m");
					break;
			}
			
			free(frame->data);
			previous_type = frame->type;
			free(frame);
		}
	} else {
		PRTERR("I/O failure for settings frame.");
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

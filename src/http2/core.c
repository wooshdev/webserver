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

#ifdef BENCHMARK
#include <time.h>
#endif

#include "base/global_settings.h"
#include "http/parser.h"
#include "utils/io.h"
#include "utils/util.h"
#include "http/header_list.h"
#include "http/response_headers.h"

#include "constants.h"
#include "dynamic_table.h"
#include "frame.h"
#include "hpack.h"
#include "huffman.h"
#include "stream.h"

#include "handling/handlers.h"

/* string compare with length */
int scomp(const char *a, const char *b, size_t len) {
	size_t i;
	for (i = 0; i < len; i++)
		if (a[i] != b[i])
			return 0;
	return 1;
}

static H2_ERROR handle_settings(frame_t *frame, setentry_t *settings) {
	setentry_t ent;
	/*
	printf("[\x1b[33mSettings\x1b[0m] \x1b[32mSize: %u Flags: 0x%x stream_id: 0x%x\n", frame->length, frame->flags, frame->r_s_id & 0xEF);
	*/
	if (frame->length > 0) {
		size_t i;
		for (i = 0; i < frame->length / 6; i++) {
			char *start = frame->data + i*6;
			ent.id = (start[0] << 8) | start[1];
			ent.value = u32(start+2);
			
			if (ent.id > HTTP2_SETTINGS_COUNT) {
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
			
			/*
			printf("[\x1b[33mSettings\x1b[0m] \x1b[32mName: %s Value: %u\n", settings_names[ent.id], ent.value);
			*/
			
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
	
	int ret = send_frame(tls, 6, FRAME_SETTINGS, 0x0, 0x0, buf);
	free(buf);
	return ret;
}

static int send_settings_ack(TLS tls) {
	return send_frame(tls, 0, FRAME_SETTINGS, FLAG_ACK, 0x0, NULL);
}

static void send_rst(TLS tls, uint32_t error) {
	printf("\x1b[33m[!] Sending RST_STREAM frame! Error: %s\x1b[0m\n", h2_error_codes[error]);
	send_frame(tls, 4, FRAME_RST_STREAM, 0, 0, (char *)&error);
}

static void send_goaway(TLS tls, uint32_t error, uint32_t stream) {
	printf("\x1b[33m[!] Sending GOWAY frame! Error: %s\x1b[0m\n", h2_error_codes[error]);
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

static void h2_callback_headers_ready(http_response_headers_t *response_headers, size_t app_data_len, void **application_data) {
	TLS tls = (TLS) application_data[0];
	frame_t *frame = (frame_t *)application_data[1];

	size_t size = 0;
	char *headers = write_headers(response_headers, &size);

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

	/*printf(" > sending HEADERS frame, len=%zu\n", pos);*/
	send_frame(tls, size, FRAME_HEADERS, FLAG_END_HEADERS, frame->r_s_id, headers);
	free(headers);
}

static void h2_handle(TLS tls, frame_t *frame, http_header_list_t *request_header_list, setentry_t *settings) {
	handler_callbacks_t *callback_info = malloc(sizeof(handler_callbacks_t));
	if (!callback_info) {
		fprintf(stderr, "h2_handle: memory allocation error!");
		return;
	}
	callback_info->headers_ready = h2_callback_headers_ready;
	callback_info->application_data_length = 2;
	callback_info->application_data = calloc(callback_info->application_data_length, sizeof(void *));
	callback_info->application_data[0] = tls;
	callback_info->application_data[1] = frame;

	http_response_t *response = http_handle_request(request_header_list, callback_info);
	free(callback_info->application_data);
	free(callback_info);

	/*printf(" > sending DATA frame, len=%zu\n", response->body_size);*/
	send_frame(tls, response->body_size, FRAME_DATA, FLAG_END_STREAM, frame->r_s_id, response->body);
	
	if (response->is_dynamic) {
		free(response->body);
		http_response_headers_destroy(response->headers);
		free(response);
	}
}

const char *get_frame_name(uint32_t type) {
	if (type > FRAME_ORIGIN)
		return "Unassigned";
	else
		return frame_types[type];
}

void http2_handle(TLS tls) {
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
	settings[5].id = 0x6;           /* SETTINGS_MAX_HEADER_LIST_SIZE */
	settings[5].value = UINT32_MAX; /* initial value: unset */
	/*
	puts("\x1b[32mbegin\x1b[0m");
	*/
	char prefacebuf[24] = { 0 };
	if (!tls_read_client_complete(tls, prefacebuf, 24) || !scomp(prefacebuf, preface, 24)) {
		PRTERR("[H2] Preface io/comparison failure.\n");
		return;
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

	h2stream_list_t *streams = h2stream_list_create(65535);

	if (GLOBAL_SETTING_origin) {
		size_t len = strlen(GLOBAL_SETTING_origin);
		char *origin_frame = malloc(2 + len);
		
		origin_frame[0] = (len >> 8) & 0xFF;
		origin_frame[1] = len & 0xFF;
		memcpy(origin_frame, GLOBAL_SETTING_origin, len);
		send_frame(tls, 2 + len, FRAME_ORIGIN, 0x0, 0x0, origin_frame);
		free(origin_frame);
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
			goto frame_end;
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
		headers->version = HTTP_VERSION_2;
		
		size_t previous_type = 0x4;

		while ((frame = readfr(tls, settings[4].value, &error))) {
			#ifdef BENCHMARK			
				clock_t start_time = clock();
			#endif

			switch (h2stream_get_state(streams, frame->r_s_id)) {
				case H2_STREAM_IDLE:
					if (frame->type == FRAME_HEADERS) {
						h2stream_set_state(streams, frame->r_s_id, H2_STREAM_OPEN);
					} else if (frame->type != FRAME_PRIORITY) {
						printf("PROTOCOL_ERROR: Client has sent a %s on a stream which is idle.\n", get_frame_name(frame->type));
						send_goaway(tls, H2_PROTOCOL_ERROR, frame->r_s_id);
						goto frame_end;
					}
					break;
				case H2_STREAM_HALF_CLOSED_LOCAL:
					if (frame->type != FRAME_WINDOW_UPDATE && frame->type != FRAME_PRIORITY && frame->type != FRAME_RST_STREAM) {
						printf("PROTOCOL_ERROR: Client has sent a %s on a stream which is half_closed (local).\n", get_frame_name(frame->type));
						send_goaway(tls, H2_STREAM_CLOSED, frame->r_s_id);
						goto frame_end;
					}
					break;
				case H2_STREAM_HALF_CLOSED_REMOTE:
					if (frame->type != FRAME_WINDOW_UPDATE && frame->type != FRAME_PRIORITY && frame->type != FRAME_RST_STREAM) {
						printf("PROTOCOL_ERROR: Client has sent a %s on a stream which is half_closed (remote).\n", get_frame_name(frame->type));
						send_goaway(tls, H2_STREAM_CLOSED, frame->r_s_id);;
						goto frame_end;
					}
					break;
				case H2_STREAM_CLOSED_STATE:
					if (frame->type != FRAME_PRIORITY) {
						if (frame->type == FRAME_WINDOW_UPDATE || frame->type == FRAME_RST_STREAM) {
							printf("TODO: Client has sent a %s on a stream which is closed (may be a short time after closing).\n", get_frame_name(frame->type));
							break;
						}
						printf("PROTOCOL_ERROR: Client has sent a %s on a stream which is closed.\n", get_frame_name(frame->type));
						send_goaway(tls, H2_STREAM_CLOSED, frame->r_s_id);
						goto frame_end;
					}
					break;
				case H2_STREAM_RESERVED_LOCAL:
					if (frame->type != FRAME_WINDOW_UPDATE && frame->type != FRAME_PRIORITY && frame->type != FRAME_RST_STREAM) {
						printf("PROTOCOL_ERROR: Client has sent a %s on a stream which is reserved (local).\n", get_frame_name(frame->type));
						send_goaway(tls, H2_PROTOCOL_ERROR, frame->r_s_id);
					}
					break;
				default:
					break;
			}

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
					
					handle_headers(frame, dynamic_table, headers);
					
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
						
						h2_handle(tls, frame, headers, settings);
						http_destroy_header_list(headers);
						headers = http_create_header_list();
						headers->version = HTTP_VERSION_2;
					}
					break;
				case FRAME_PRIORITY: {
					if (frame->length != 5) {
						/* connection error */
						send_rst(tls, H2_FRAME_SIZE_ERROR);
						goto frame_end;
					}
					
					char e = frame->data[0] & 0x10;
					uint32_t stream_dependency = u32(frame->data) & BITS31;
					uint8_t weight = frame->data[4];
					
					printf("\x1b[35mPriority> \x1b[0mstream=%u e=%s s-depend=%u weight=%hhi\n", frame->r_s_id & BITS31, e ? "true" : "false", stream_dependency, weight);
				} break;
				case FRAME_RST_STREAM:
					/*
					fputs("\x1b[33mEnd (semi-gracefully) requested, ", stdout);
					if (frame->length == 4) {
						printf("reason: %s\x1b[0m\n", h2_error_codes[u32(frame->data)]);
					} else {
						puts("but the reason was corrupted.");
					}
					*/
					/**
					 * If a RST_STREAM frame is received with a stream identifier of 0x0,
					 * the recipient MUST treat this as a connection error (Section 5.4.1)
					 * of type PROTOCOL_ERROR */
					if (frame->r_s_id == 0x0) {
						send_goaway(tls, H2_PROTOCOL_ERROR, 0x0);
						goto frame_end;
					}
					h2stream_set_state(streams, frame->r_s_id, H2_STREAM_CLOSED_STATE);
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
						goto frame_end;
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
						goto frame_end;
					}
					uint32_t wsi = u32(frame->data) & 0xEFFFFFFF;
					if (window_size == wsi) {
						/* connection error */
						puts("Illegal Window Size! (i.e. a PROTOCOL_ERROR)");
						send_rst(tls, H2_PROTOCOL_ERROR);
						goto frame_end;
					}
					/*
					printf(" > Size Increment: 0x%X or 0x%X = %u %u\n", u32(frame->data), wsi, u32(frame->data), wsi);
					printf(" > Additional data: a=0x%hhx b=0x%hhx c=0x%hhx d=0x%hhx\n", frame->data[0], frame->data[1], frame->data[2], frame->data[3]);
					*/
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

			if (frame->flags & FLAG_END_STREAM) {
				if (frame->r_s_id == 0) {
					puts("TODO: END_STREAM flag set on stream 0!");
				} else {
					h2stream_set_state(streams, frame->r_s_id, H2_STREAM_HALF_CLOSED_REMOTE);
				}
			}

#ifdef BENCHMARK
			printf("\033[0;33mFrame> \033[0;32m%s \033[0mtook \033[0;35m%.3f ms\033[0m to process...\n", frame_types[frame->type], (clock()-start_time)/1000.0);
#endif

			free(frame->data);
			previous_type = frame->type;
			free(frame);
			continue;
			
			frame_end:
			free(frame->data);
			previous_type = frame->type;
			free(frame);
			goto end;
		}
		
		if (error != H2_NO_ERROR) {
			send_goaway(tls, error, 0x0);
			fputs("\x1b[31m[H2] Error: ", stdout);
			if (error < H2_ERROR_CODE_COUNT)
				puts(h2_error_codes[error]);
			else
				printf("Unknown or invalid error: 0x%X\n", error);
			fputs("\x1b[0m", stdout);
		} else {
			
		}
	} else {
		if (error == H2_NO_ERROR) {
			PRTERR("I/O failure for settings frame.");
		} else {
			send_goaway(tls, error, 0x0);
		}
	}
	
	end:
	free(settings);
	if (streams)
		h2stream_list_destroy(streams);
	if (dynamic_table)
		dynamic_table_destroy(dynamic_table);
	if (headers)
		http_destroy_header_list(headers);
	return;
}

int http2_setup() {
	return huff_setup();
}

void http2_destroy() {
	huff_destroy();
}

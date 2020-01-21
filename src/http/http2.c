#include "http2.h"
#include <string.h>
#include "common.h"
#include "parser.h"
#include "../utils/io.h"
#include <stdlib.h>
#include <stdint.h>
#include "http2constants.c"
#include "http2frame.c"
#include "http2hpack.c"

/* settings entry (RFC 7540 Section 6.5.1)*/
typedef struct {
	uint16_t id;	/* identifier */
	uint32_t value;
} setentry_t;

/* string compare with length */
int scomp(const char *a, const char *b, size_t len) {
	size_t i;
	for (i = 0; i < len; i++)
		if (a[i] != b[i])
			return 0;
	return 1;
}

static int handle_settings(frame_t *frame, setentry_t *settings) {
	setentry_t ent;
	if (frame->length > 0) {
		size_t i;
		for (i = 0; i < frame->length / 6; i++) {
			char *start = frame->data + i*6;
			ent.id = (start[0] << 8) | start[1];
			ent.value = u32(start+2);
			
			if (ent.id >= sizeof(settings_ids)/sizeof(settings_ids[0])) {
				fprintf(stderr, "[H2] Invalid setting identifier: %hu with value %u\n", ent.id, ent.value);
				return 0;
			}
			
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
	return send_frame(tls, 0, 0x4, 0x1, 0, NULL);
}

static void send_rst(TLS tls, uint32_t error) {
	PRTERR("sending rst frame");
	send_frame(tls, 4, 0x3, 0, 0, (char *)&error);
}

http_request_t http2_parse(TLS tls) {
	http_request_t req = { 0 };
	
	uint32_t window_size = UINT16_MAX;
	setentry_t *settings = calloc(SETTINGS_COUNT, sizeof(setentry_t));
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
	if (settings) {
		if (frame->type != 0x4) {
			PRTERR("[H2] Protocol error: first frame wasn't a settings frame!");
			goto end;
		}
		
		handle_settings(frame, settings);
		free(frame->data);
		free(frame);
		
		send_settings(tls);
		
		while ((frame = readfr(tls))) {
			switch (frame->type) {
				case 0x1:
					handle_headers(frame);
					break;
				case 0x2: {/* PRIORITY */
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
				case 0x3: /* RST */
					fputs("\x1b[33mEnd (semi-gracefully) requested, ", stdout);
					if (frame->length == 4) {
						printf("reason: %s\x1b[0m\n", error_codes[u32(frame->data)]);
					} else {
						puts("but the reason was corrupted.");
					}
					goto end;
					break;
				case 0x7: /* GOAWAY */
					printf("\x1b[31m > GOAWAY ErrorCode=%s\x1b[0m\n", error_codes[u32(frame->data+4)]);
					break;
				case 0x8: /* WINDOW_UPDATE */
					if (frame->length != 4) {
						/* connection error */
						send_rst(tls, H2_FRAME_SIZE_ERROR);
						goto end;
					}
					if ((window_size = u32(frame->data)) == 0) {
						/* connection error */
						send_rst(tls, H2_PROTOCOL_ERROR);
						goto end;
					}
					break;
				default:
					puts("\x1b[31m > Type unknown.\x1b[0m");
					break;
			}
			
			free(frame->data);
			free(frame);
		}
	} else {
		PRTERR("I/O failure for settings frame.");
	}
	
	end:
	puts("\x1b[32mend\x1b[0m");
	return req;
}

int http2_setup() {
	return huff_setup();
}

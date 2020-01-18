#include "http2.h"
#include <string.h>
#include "common.h"
#include "parser.h"
#include "../utils/io.h"
#include <stdlib.h>
#include <stdint.h>

typedef struct {
	unsigned int length : 24;
	unsigned char type;
	unsigned char flags;
	uint32_t r_s_id;/* r + stream identifier */
	char *data;
} frame_t;

/* predefined functions */
frame_t *readfr(TLS tls);

/* settings entry (6.5.1)*/
typedef struct {
	uint16_t id;	/* identifier */
	uint32_t value;
} setentry_t;

#define SETTINGS_COUNT 6

 /* 6.5.2 */
static const char *settings_ids[] = { NULL, "SETTINGS_HEADER_TABLE_SIZE", "SETTINGS_ENABLE_PUSH", "SETTINGS_MAX_CONCURRENT_STREAMS", "SETTINGS_INITIAL_WINDOW_SIZE", "SETTINGS_MAX_FRAME_SIZE", "SETTINGS_MAX_HEADER_LIST_SIZE" };

static const char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

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
			ent.value = (start[2] << 24) | (start[3] << 16) | (start[4] << 8) | start[5];
			
			if (ent.id >= sizeof(settings_ids)/sizeof(settings_ids[0])) {
				fprintf(stderr, "[H2] Invalid setting identifier: %hu with value %u\n", ent.id, ent.value);
				return 0;
			}
			
			settings[ent.id-1].value = ent.value;
		}
	}
	
	return 1;
}

http_request_t http2_parse(TLS tls) {
	http_request_t req = { 0 };
	
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
	
	puts("begin");
	printf("%zi\n", sizeof(frame_t));
	
	char prefacebuf[24] ={ 0 };
	if (!tls_read_client_complete(tls, prefacebuf, 24) || !scomp(prefacebuf, preface, 24)) {
		fputs("[H2] Preface io/comparison failure.\n", stderr);
		return req;
	}
	
	frame_t *frame = readfr(tls);
	if (settings) {
		printf("frame > type=0x%X length=%i\n", frame->type, frame->length);
		handle_settings(frame, settings);
		
		free(frame->data);
		free(frame);
	} else {
		puts("I/O failure for settings frame.");
	}
	
	puts("end");
	return req;
}

frame_t *readfr(TLS tls) {
	frame_t *f = malloc(sizeof(frame_t));
	if (!f)
		return NULL;
	unsigned char parts[3];
	if (!tls_read_client_complete(tls, (char*)parts, 3)) {
		free(f);
		return NULL;
	}
	f->length = (parts[0]<<16) | (parts[1] << 8) | parts[2];
	
	if (!tls_read_client_complete(tls, (char *)&f->type, sizeof(f->type))) {
		free(f);
		return NULL;
	}
	if (!tls_read_client_complete(tls, (char *)&f->flags, sizeof(f->flags))) {
		free(f);
		return NULL;
	}
	if (!tls_read_client_complete(tls, (char *)&f->r_s_id, sizeof(f->r_s_id))) {
		free(f);
		return NULL;
	}
	
	if (!(f->data = malloc(sizeof(char) * f->length))) {
		free(f);
		return NULL;
	}
	
	if (!tls_read_client_complete(tls, f->data, f->length)) {
		free(f->data);
		free(f);
		return NULL;
	}
	
	return f;
}

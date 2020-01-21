#include "huffman.c"

/**
 * Symbols conforming RFC 7541.
 */

/**
 * Definition:
 *   Appendix A. Static Table Definition
 *
 * Notes:
 *   Header fields with values are seperated by a '$' character, a.k.a DOLLAR_SIGN.
 */
static const char *hstatic_table[] = {
	NULL,
	":authority",
	":method$GET",
	":method$POST|",
	":path$/",
	":path$/index.html",
	":scheme$http",
	":scheme$https",
	":status$200",
	":status$206",
	":status$304",
	":status$400",
	":status$404",
	":status$500",
	"accept-charset",
	"accept-encoding$gzip, deflate",
	"accept-language",
	"accept-ranges",
	"accept",
	"access-control-allow-origin",
	"age",
	"allow",
	"authorization",
	"cache-control",
	"content-disposition",
	"content-encoding",
	"content-language",
	"content-length",
	"content-location",
	"content-range",
	"content-type",
	"cookie",
	"date",
	"etag",
	"expect",
	"expires",
	"from",
	"host",
	"if-match",
	"if-modified-since",
	"if-none-match",
	"if-range",
	"if-unmodified-since",
	"last-modified",
	"link",
	"location",
	"max-forwards",
	"proxy-authenticate",
	"proxy-authorization",
	"range",
	"referer",
	"refresh",
	"retry-after",
	"server",
	"set-cookie",
	"strict-transport-security",
	"transfer-encoding",
	"user-agent",
	"vary",
	"via",
	"www-authenticate"
};

static char *dup_str(const char *src, size_t length) {
	char *copy = malloc(length * sizeof(char));
	if (!copy)
		return NULL;
	size_t i;
	for (i = 0; i < length; i++)
		copy[i] = src[i];
	return copy;
}

/**
 * Description:
 *   Parses a integer as per RFC 7541 Section 5.1
 *
 * Parameters:
 *   char *
 *     The source to get the octets from.
 *   size_t *
 *     The integer to store the amount of
 *     used octets from the "stream" in.
 *     This amount is added, not replaced.
 *
 * Return Value:
 *   The integer.
 */
size_t parse_int(char *stream, size_t *offset_out, size_t n) {
	/* fb: first-byte */
	char fb = stream[0] & 0x1F;
	if (fb < 31) {
		*offset_out+=1;
		return fb;
	}

	*offset_out+=1;
	return 648;
}

static void handle_headers(frame_t *frame) {
	puts("\x1b[93m>>> \x1b[94mHPACK\x1b[93m <<<\x1b[0m");
	size_t offset = 0;
	uint32_t padding = frame->flags & FLAG_PADDED ? frame->data[offset++] : 0;
	uint32_t dependency = 0;
	uint8_t weight = 0;
	if (frame->flags & FLAG_PRIORITY) {
		dependency = u32(frame->data + offset);
		weight = frame->data[offset + 1];
		offset += 5;
	}

	size_t packl = frame->length - offset - padding;
	char *data = dup_str(frame->data + offset, packl);

	size_t i = 0;
	for (i = 0; i < packl;) {
		printf("\x1b[32m > \x1b[34m(%03zu) \x1b[33m0x%02hhx int=%hhu parsed=%zu\x1b[0m ", i, data[i], data[i], parse_int(frame->data+i, &i, 60));
		if (data[i] == '\r')
			puts("c=CR");
		else
			printf("c=%c\n", data[i]);
	}

	for (i = 0; i < packl; i++) {
		char c = data[i];
		if (c > 127) {
			puts("\tindexed");
		} else if (c > 64) {
			puts("\tliteral field");
		} else if (c == 64) {
			puts("\tliteral field with incremental indexing");
		} else if (c > 0 && c < 16) {
			puts("\tliteral field with indexed new name");
			
			char val_len = data[++i];

			if (val_len > 127) {
				puts("\t\thuffman");
			} else {
				printf("\t\tplain: 0x%02x\n", val_len&0xEF);
			}
		} else if (c == 0) {
			puts("\tliteral field without indexing");
			char val_len = data[++i];

			if (val_len > 127) {
				puts("\t\thuffman");
			} else {
				printf("\t\tplain: 0x%02x\n", val_len&0xEF);
			}
		} else {
			printf("\tother? u=%hhu\n", c);
		}
	}
}

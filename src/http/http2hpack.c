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
	":method$POST",
	":path$/",
	":path$/index.html",
	":scheme$http",
	":scheme$https",
	":status$200",
	":status$204",
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
/* fb: first-byte */
/*size_t parse_int(char *stream, size_t *offset_out, size_t n) {
	
	char fb = stream[0] & 0x1F;
	if (fb < 31) {
		*offset_out+=1;
		return fb;
	}

	*offset_out+=1;
	return 648;
}*/

size_t parse_int(const char *stream, 
								 size_t *out_octets_used, /* how many bytes were used*/
								 size_t n
								) {
	/* we can just use the first octet */
	/* this is essentialy the same as stream < pow(2, n),
	 * but is faster and looks cooler */
	char i = stream[0] & (1 << (n-1));
	if (i == 0) {
		*out_octets_used = 1;
		char c = stream[0] & ~(1<<n);
		return (size_t)c;
	} else {
		printf("ok parse_int need fix, stream=%hhu %hhu\n", stream[0], i);
		return 0;
	}
}

static char *parse_string(const char *data, size_t *octets_used, size_t *length) {
	size_t ioctets_used;
	size_t ilength = parse_int(data, &ioctets_used, 7);
	printf("Value length: %zu (huffman=%X)\n", *length, (data[0] & 0x80) >> 7);
	*octets_used = ilength + ioctets_used;
  
	/* determine type of string (huffman, normal) */
	if ((data[0] & 0x80) >> 7) {
		char *huff = huff_decode(data + ioctets_used, ilength);
		printf(" > parsed string='%s' (ioctets_used=%zu ilength=%zu octets_used=%zu)\n", huff, ioctets_used, ilength, *octets_used);
		return huff;
	} else {
    char *pdata = malloc(sizeof(char) * (ilength + 1));
    pdata[ilength] = 0;
    memcpy(pdata, data+ioctets_used, ilength);
    printf(" > non_huff='%s'\n", pdata);
    printf(" >> ilength=%zu otects_used=%zu [0]=%hhX [1]=%hhX data[first]=%hhX data[first-1]=%hhX\n", *length, *octets_used, pdata[0], pdata[1], data[*octets_used], data[*octets_used - 1]);
    return pdata;
	}
	return NULL;
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
	printf("[HeaderBlockInfo] Dependency=%u Weight=%u\n", dependency, weight);

	size_t packl = frame->length - offset - padding;
	char *data = dup_str(frame->data + offset, packl);

	size_t i = 0;
	for (i = 0; i < packl; i++) {
		printf("\x1b[32m > \x1b[34m(%03zu) \x1b[33m0x%02hhx int=%hhu\x1b[0m\n", i, data[i], data[i]/*, parse_int(frame->data+i, &i, 60)*/);
	}

	for (i = 0; i < packl; i++) {
		unsigned char c = data[i];
		printf("first_octet=0x%02hhX", c);
		if (c > 127) {
			puts("\tIndexed Header Field");
			
			/*printf("%hhu => %hhu\n", data[i], data[i] & ~0x80);
			data[i] &= ~0x80;*/
			
			size_t octets_used = 0;
			size_t pos = parse_int(data+i, &octets_used, 7);
			/*printf("Pos: %zu\n", pos);*/
			
			const char *statentry = hstatic_table[pos];
			char *sign_position = strchr(statentry, '$');
			
			if (!sign_position) {
				printf("\nsign_entry is null!\nfield=%s\n\n", statentry);
			}
			
			size_t key_size = (sign_position - statentry) + 1;
			size_t value_size = strlen(statentry) - (sign_position - statentry) + 1;
			
			char *key = malloc(sizeof(char) * key_size);
			char *value = malloc(sizeof(char) * value_size);
			
			strcpy(key, statentry);
			key[key_size - 1] = 0;
			
			strcpy(value, sign_position+1);
			value[value_size - 1] = 0;
			
			printf("DBG_DAT statentry=%p, sign_position=%p, key=%p value=%p knt=%zi vnt=%zi\n", statentry, sign_position, key, value, statentry - sign_position, sign_position - statentry);
			printf("\x1b[33m[Header] Key='%s' Value='%s'\x1b[0m\n", key, value);
			
			free(key);
			free(value);
			
		} else if (c > 64) {
			/* this value should be added to the list,
			 but also added to the dynamic table (this is like caching headers) */
      /* 01??????*/
			puts("\tLiteral Header Field with Incremental Indexing -- Indexed Name");
      
			
      size_t octets_used = 0;
			size_t length = 0;
			size_t pos = parse_int(data + i, &octets_used, 6);
      printf("\t> pos=%zu a.k.a. key='%s'\n", pos, hstatic_table[pos]);
			i += octets_used;
      
      char *value = parse_string(data + i, &octets_used, &length);
			printf("\x1b[33m[Header] Key='%s' Value='%s'\x1b[0m\n", hstatic_table[pos], value);
			free(value);
			i += octets_used - 1; /* we have to subtract 1 because the for loop adds one for us */
      
		} else if (c == 64) {
			/* this value should be added to the list,
			 but also added to the dynamic table (this is like caching headers) */
			puts("\tLiteral Header Field with Incremental Indexing -- New Name");
			/*both key and value are supplied*/
			size_t length, octets_used;
      
      i+=1;
			char *hkey = parse_string(data+i, &octets_used, &length);
     
      i += octets_used;
			char *hval = parse_string(data+i, &octets_used, &length);
      i += octets_used - 1; /* we have to subtract 1 because the for loop adds one for us */
			printf("\x1b[33m[Header] Key='%s' Value='%s'\x1b[0m\n", hkey, hval);
			free(hkey);
			free(hval);
		} else if (c > 0 && c < 16) {
			puts("\tLiteral Header Field without Indexing -- Indexed Name");
			
			size_t octets_used = 0;
			size_t length = 0;
			size_t pos = parse_int(data+i, &octets_used, 6);

			printf("Header: %s (pos=%zu)\n", hstatic_table[pos], pos);
			i += octets_used;

			char *hdata = parse_string(data+i, &octets_used, &length);
			free(hdata);
			i += octets_used - 1;
			
		} else if (c == 0) {
			puts("\tLiteral Header Field without Indexing -- New Name");
			char val_len = data[++i];

			if (val_len > 127) {
				puts("\t\thuffman");
			} else {
				printf("\t\tplain: 0x%02x\n", val_len&0xEF);
			}
		} else {
			printf("\tother? i=%hhu\n", c);
		}
	}
}

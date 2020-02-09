/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains untility functions for the decompression & parsing of a HPACK blob.
 */
#include "constants.h"
#include "dynamic_table.h"
#include "hpack.h"
#include "huffman.h"
#include "../http/header_list.h"
#include "../utils/util.h"

#include <stdlib.h>
#include <string.h>

/* to enable logging: */
/*
#define HPACK_LOGGING_TYPE
and/or
#define HPACK_LOGGING_KEY_VALUE
and/or
#define HPACK_LOGGING_VERBOSE
and/or
#define HPACK_LOGGING_DUMP
and/or
#define HPACK_LOGGING_ERROR
and/or
#define HPACK_LOGGING_RESULTS
*/

#define HPACK_LOGGING_ERROR

/*HPACK_LOGGING_VERBOSE enables everything */
#ifdef HPACK_LOGGING_VERBOSE
#define HPACK_LOGGING_KEY_VALUE
#define HPACK_LOGGING_TYPE
#define HPACK_LOGGING_DUMP
#define HPACK_LOGGING_ERROR
#endif

char *dup_str(const char *src, size_t length) {
	char *copy = malloc(length * sizeof(char));
	if (!copy)
		return NULL;
	size_t i;
	for (i = 0; i < length; i++)
		copy[i] = src[i];
	return copy;
}

/* duplicate string + NULL-termination */
char *tdup_str(const char *src, size_t length) {
	char *copy = malloc((length+1) * sizeof(char));
	if (!copy)
		return NULL;
	copy[length] = 0;
	size_t i;
	for (i = 0; i < length; i++)
		copy[i] = src[i];
	return copy;
}

size_t parse_int(const char *stream, size_t *out_octets_used, size_t n) {
	/* we can just use the first octet */
	/* this is essentialy the same as stream < pow(2, n),
	 * but is faster and looks cooler */
	size_t next_n_bits = (1 << n)-1;
	size_t i = (stream[0] & 0xFF) & ~(1<<n);
	#ifdef HPACK_LOGGING_VERBOSE
	printf("parse int i=%zu s=%hhu n=%zu max=%zu other=%hhx\n", i, stream[0], n, next_n_bits, stream[0] & ~(1<<n));
	#endif
	if (i < next_n_bits) {
		*out_octets_used = 1;
		char c = stream[0] & ~(1<<n);
		return (size_t)c;
	} else {
		#ifdef HPACK_LOGGING_VERBOSE
		printf("\x1b[31mok parse_int need fix, stream=%hhu %zu\x1b[0m\n", stream[0], i);
		#endif
		size_t m = 0, b, position = 0;
		do {
			b = stream[++position] & 0xFF;
			#ifdef HPACK_LOGGING_VERBOSE
			printf(" > b=%zu\n", b);
			printf("(m = %zu) 2^m = %i\n", m, (1 << m));
			#endif
			i += (b & 127) * (1 << m);
			m += 7;
		} while ((b & 128) == 128);
		
		#ifdef HPACK_LOGGING_VERBOSE
		printf("parse_int_fix> i=%zu m=%zu position=%zu\n", i, m, position);
		#endif
		
		*out_octets_used = position + 1;
		return i;
	}
}

char *parse_string(const char *data, size_t *octets_used, size_t *length) {
	size_t ioctets_used;
	size_t ilength = parse_int(data, &ioctets_used, 7);
	/*printf("Value length: %zu (huffman=%X)\n", *length, (data[0] & 0x80) >> 7);*/
	*octets_used = ilength + ioctets_used;
	
	/* determine type of string (huffman, normal) */
	if ((data[0] & 0x80) >> 7) {
		char *huff = huff_decode(data + ioctets_used, ilength);
		/*printf(" > parsed string='%s' (ioctets_used=%zu ilength=%zu octets_used=%zu)\n", huff, ioctets_used, ilength, *octets_used);*/
		return huff;
	} else {
		char *pdata = malloc(sizeof(char) * (ilength + 1));
		pdata[ilength] = 0;
		memcpy(pdata, data+ioctets_used, ilength);
		/*
		printf(" > non_huff='%s'\n", pdata);
		printf(" >> ilength=%zu otects_used=%zu [0]=%hhX [1]=%hhX data[first]=%hhX data[first-1]=%hhX\n", *length, *octets_used, pdata[0], pdata[1], data[*octets_used], data[*octets_used - 1]);
		*/
		return pdata;
	}
	return NULL;
}

void handle_headers(frame_t *frame, dynamic_table_t *dynamic_table, http_header_list_t *list) {
	#ifdef HPACK_LOGGING_DUMP
	puts("\x1b[93m>>> \x1b[94mHPACK\x1b[93m <<<\x1b[0m");
	#endif
	size_t offset = 0;
	uint32_t padding = frame->flags & FLAG_PADDED ? frame->data[offset++] : 0;
	
	#ifdef HPACK_LOGGING_VERBOSE
	uint32_t dependency = 0;
	uint8_t weight = 0;
	#endif
	
	if (frame->flags & FLAG_PRIORITY) {
		#ifdef HPACK_LOGGING_VERBOSE
		dependency = u32(frame->data + offset);
		weight = frame->data[offset + 1];
		#endif
		offset += 5;
	}
	#ifdef HPACK_LOGGING_VERBOSE
	printf("[HeaderBlockInfo] Dependency=%u Weight=%u\n", dependency, weight);
	#endif

	size_t packl = frame->length - offset - padding;
	char *data = dup_str(frame->data + offset, packl);

	size_t i = 0;
	#ifdef HPACK_LOGGING_DUMP
	for (i = 0; i < packl; i++) {
		printf("\x1b[32m > \x1b[34m(%03zu) \x1b[33m0x%02hhx int=%hhu\x1b[0m\n", i, data[i], data[i]/*, parse_int(frame->data+i, &i, 60)*/);
	}
	#endif

	size_t octets_used = 0;
	
	size_t count = 0;
	for (i = 0; i < packl; i++) {
		unsigned char c = data[i];
		count++;
		
		#ifdef HPACK_LOGGING_TYPE
		printf("(0x%02hhX)", c);
		#endif
		
		if (c > 127) {
			#ifdef HPACK_LOGGING_TYPE
			puts("\tIndexed Header Field");
			#endif
			size_t pos = parse_int(data+i, &octets_used, 7);
			#ifdef HPACK_LOGGING_VERBOSE
			printf(" > Pos: %zu max-int=%zu\n", pos, 62+dynamic_table->index_last);
			#endif
			
			lookup_t result = dynamic_table_get(dynamic_table, pos);
			const char *indexed_name = result.static_e ? result.static_e : (result.dynamic ? result.dynamic->key : NULL);
			if (!indexed_name) {
				goto error_label;
			}
			
			char *key, *value;
			if (result.static_e) {
				char *sign_position = strchr(indexed_name, '$');
				if (!sign_position) {
					printf("\nsign_entry is null!\nfield=%s\n\n", indexed_name);
					goto error_label;
				}
				
				size_t key_size = (sign_position - indexed_name) + 1;
				size_t value_size = strlen(indexed_name) - (sign_position - indexed_name) + 1;
				
				key = malloc(sizeof(char) * key_size);
				value = malloc(sizeof(char) * value_size);
				
				memcpy(key, indexed_name, key_size - 1);
				key[key_size - 1] = 0;
				
				strcpy(value, sign_position+1);
				value[value_size - 1] = 0;
				
				#ifdef HPACK_LOGGING_KEY_VALUE
				printf("\x1b[33m[Header] Key='%s' Value='%s'\x1b[0m\n", key, value);
				#endif
				
				/* even though we got the string from the static table, 
				   we are duplicating it so it isn't cached */
				http_header_list_add(list, key, value, HTTP_HEADER_NOT_CACHED, pos);
			} else if (result.dynamic) {
				/**
				 * The dynamic table SHOULD NOT be destroyed before these pointers are out of use,
				 * because otherwise these pointers will point to disowned data.
				 */
				key = result.dynamic->key;
				value = result.dynamic->value;
				
				#ifdef HPACK_LOGGING_KEY_VALUE
				printf("\x1b[33m[Header] Key='%s' Value='%s'\x1b[0m\n", key, value);
				#endif
				
				http_header_list_add(list, key, value, HTTP_HEADER_CACHED, pos);
			} else {
				printf("Static+Dynamic table out-of-bounds error for pos=%zu\n", pos);
				goto error_label;
			}
		} else if (c > 64) {
			/* this value should be added to the list,
			 but also added to the dynamic table (this is like caching headers) */
			/* 01??????*/
			#ifdef HPACK_LOGGING_TYPE
			puts("\tLiteral Header Field with Incremental Indexing -- Indexed Name");
			#endif
			size_t length = 0;
			size_t pos = parse_int(data + i, &octets_used, 6);
			i += octets_used;
			
			/** Get the key ("name") out of either the static or dynamic table */
			lookup_t result = dynamic_table_get(dynamic_table, pos);
			const char *indexed_name = result.static_e ? result.static_e : result.dynamic->key;
			if (!indexed_name) {
				goto error_label;
			}
			
			char *key = NULL;
			char *sign_position = strchr(indexed_name, '$');
			if (sign_position) {
				key = tdup_str(indexed_name, sign_position - indexed_name);
			} else {
				key = strdup(indexed_name);
			}

			char *value = parse_string(data + i, &octets_used, &length);
			i += octets_used - 1; /* we have to subtract 1 because the for loop adds one for us */
			
			#ifdef HPACK_LOGGING_KEY_VALUE
			printf("\x1b[33m[Header] Key='%s' Value='%s'\x1b[0m\n", key, value);
			#endif
			
			dynamic_table_add(dynamic_table, key, value);
			http_header_list_add(list, key, value, HTTP_HEADER_CACHED, pos);
		} else if (c == 64) {
			/* this value should be added to the list,
			 but also added to the dynamic table (this is like caching headers) */
			 #ifdef HPACK_LOGGING_TYPE
			puts("\tLiteral Header Field with Incremental Indexing -- New Name");
			#endif
			/*both key and value are supplied*/
			size_t length;
			
			i+=1;
			char *hkey = parse_string(data+i, &octets_used, &length);
			
			i += octets_used;
			char *hval = parse_string(data+i, &octets_used, &length);
			
			i += octets_used - 1; /* we have to subtract 1 because the for loop adds one for us */
			#ifdef HPACK_LOGGING_KEY_VALUE
			printf("\x1b[33m[Header] Key='%s' Value='%s'\x1b[0m\n", hkey, hval);
			#endif
			
			dynamic_table_add(dynamic_table, hkey, hval);
			http_header_list_add(list, hkey, hval, HTTP_HEADER_CACHED, 0);
			
		} else if (c > 0 && c < 16) {
			#ifdef HPACK_LOGGING_TYPE
			puts("\tLiteral Header Field without Indexing -- Indexed Name");
			#endif
			
			size_t length = 0;
			size_t pos = parse_int(data+i, &octets_used, 6);
			
			/** Get the key ("name") out of either the static or dynamic table */
			lookup_t result = dynamic_table_get(dynamic_table, pos);
			const char *indexed_name = result.static_e ? result.static_e : result.dynamic->key;
			if (!indexed_name) {
				goto error_label;
			}
			
			char *key = NULL;
			char *sign_position = strchr(indexed_name, '$');
			if (sign_position) {
				key = tdup_str(indexed_name, sign_position - indexed_name);
			} else {
				key = strdup(indexed_name);
			}

			#ifdef HPACK_LOGGING_VERBOSE
			printf("\x1b[33m [Header] %s (pos=%zu)\n", key, pos);
			#endif
			i += octets_used;

			char *value = parse_string(data+i, &octets_used, &length);
			#ifdef HPACK_LOGGING_KEY_VALUE
			printf("\x1b[33m [Header] Key='%s' Value='%s' (pos=%zu)\n", key, value, pos);
			#endif
			http_header_list_add(list, key, value, HTTP_HEADER_NOT_CACHED, pos);
			i += octets_used - 1;
			
		} else if (c == 0) {
			#ifdef HPACK_LOGGING_TYPE
			puts("\tLiteral Header Field without Indexing -- New Name");
			#endif
			
			size_t length;
			
			i+=1;
			char *hkey = parse_string(data+i, &octets_used, &length);
			#ifdef HPACK_LOGGING_VERBOSE
			printf("Test> HKEY='%s', length=%zu\n", hkey, length);
			#endif
			
			i += octets_used;
			char *hval = parse_string(data+i, &octets_used, &length);
			#ifdef HPACK_LOGGING_VERBOSE
			printf("Test> HVAL='%s', length=%zu\n", hval, length);
			
			printf("Bytes: 0x%hhx 0x%hhx 0x%hhx\n", data[i+octets_used-2], data[i+octets_used-1], data[i+octets_used]);
			#endif
			i += octets_used - 1; /* we have to subtract 1 because the for loop adds one for us */
			#ifdef HPACK_LOGGING_KEY_VALUE
			printf("\x1b[33m[Header] Key='%s' Value='%s'\x1b[0m\n", hkey, hval);
			#endif
			
			http_header_list_add(list, hkey, hval, HTTP_HEADER_NOT_CACHED, 0);
		} else if (c > 32 && c < 64) {
			/* dynamic table size update Section 6.3*/
			#ifdef HPACK_LOGGING_TYPE
			puts("\tDynamic Table Size Update");
			#endif
			
			#ifdef HPACK_LOGGING_VERBOSE
			size_t size =
			#endif
			parse_int(data+i, &octets_used, 5);
			i += octets_used -1;
			#ifdef HPACK_LOGGING_VERBOSE
			printf("\t> New size: %zu\n", size);
			#endif
		} else {
			#ifdef HPACK_LOGGING_ERROR
			printf("\tother? i=%hhu\n", c);
			#endif
		}
	}
	#ifdef HPACK_LOGGING_RESULTS
	printf("\nEnd of headers!\n\tpackl: %zu\n\ti: %zu\n\tcount: %zu\n", packl, i, count);
	#endif
	
	#ifdef HPACK_LOGGING_VERBOSE
	printf("Flag set: %s\n", (frame->flags & FLAG_END_HEADERS ? "true" : "false"));
	
	printf("DynamicTable (index_last=%zu size=%zu client_max_size=%zu)\n", dynamic_table->index_last, dynamic_table->size, dynamic_table->client_max_size);
	if (dynamic_table->index_last != dynamic_table->client_max_size) {
		for (i = 0; i < dynamic_table->index_last+1; i++) {
			printf("> DynamicTable (%zu) Key='%s' Value='%s'\n", i, dynamic_table->entries[i]->key, dynamic_table->entries[i]->value);
		}
	}
	#endif
	
	return;
	
error_label:
	#ifdef HPACK_LOGGING_ERROR
	puts("Parsing error encountered.");
	#endif
	return;
}
 

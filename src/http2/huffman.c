/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains Huffman decoding/decompression functions.
 */
#include "huffman_table.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct hnode_t {
	struct hnode_t *left;
	struct hnode_t *right;
	/* should only be accessed when left or right are NULL*/
	size_t value;
} hnode_t;

static hnode_t *tree = NULL;

/*#define LOG_BITS*/

/**
 * TODOs:
 * 1. Ensure no data is read out side of the stream.
 * 2. Ensure no NULL leafs are taken.
 * 3. Comply to the rules of RFC7541 Section 5.2 (at the end):
 * 		"A padding not corresponding to the most significant bits of the code for the 
 *		 EOS symbol MUST be treated as a decoding error.  A Huffman-encoded string 
 *		 literal containing the EOS symbol MUST be treated as a decoding error."
 */
char *huff_decode(const char *stream, const size_t len) {
	size_t bp = 0; /* bit position */
	char *out = malloc(256);
	out[255] = 0;
	
	/*
	printf("first 0x%hhX%hhX%hhX%hhX\n", stream[0], stream[1], stream[2], stream[3]);
  printf("huff_decode len=%zu\n", len);
  */
  
	size_t i = 0;
	while (bp/8 < len && i < 255) {
		hnode_t *cur = tree; /* current node */
		do {
			size_t p = 7 - (bp % 8);
			/* bit state */
			size_t bs = (stream[bp/8] & (1<<p)) >>p;
		
#ifdef LOG_BITS
			printf("(l=%zu bp8=%zu i=%zu bp=%zu", len, bp/8, i, bp);
			printf(", bap=0x%hhX pos=%zu bs=%zu) bit: %zu\n", stream[bp/8], bp%8, p, bs);
#endif /* LOG_BITS */
			
			if (!cur->left || !cur->right) {
				out[i++] = cur->value;
				
#ifdef LOG_BITS
				printf(" > value=%zu %c bp=%zu i=%hhX\n", cur->value, cur->value, bp, stream[bp / 8]);
				printf("%zu> %hhu (%c)\n", i, cur->value, cur->value);
#endif /* LOG_BITS */
				
				break;
			}
			
			if (bs) {
				cur = cur->right;
				/*printf("1");*/
			} else {
				cur = cur->left;
				/*printf("0");*/
			}
			bp+=1;
		} while (bp/8 < len && i < 255);
	}
	/* null-terminate string. */
	out[i] = 0;
	#ifdef LOG_BITS
	printf("bp=%zu strindex=%zu ", bp, bp/8);
	printf("last bytes used: (0x%hhx is after 0x%hhx)\n", stream[bp/8], stream[(bp/8)-1]);
	#endif /* LOG_BITS */
	return out;
}

int huff_setup() {
	tree = calloc(1, sizeof(hnode_t));
	if (!tree)
		return 0;
	
	size_t i, j;
	for (i = 0; i < sizeof(http2_huffman_tree) / sizeof (http2_huffman_tree[0]); i++) {
		const char *text = http2_huffman_tree[i];
		hnode_t *current = tree;
		/* setting the value initially to a predefined value. this is useful for debugging.*/
		current->value = 648;
		
		for (j = 0; j < strlen(text); j++) {
			if (text[j] == '0') {
				if (!current->left) {
					current = current->left = calloc(1, sizeof(hnode_t));
				} else {
					current = current->left;
				}
			} else if (text[j] == '1') {
				if (!current->right) {
					current = current->right = calloc(1, sizeof(hnode_t));
				} else {
					current = current->right;
				}
			} else {
				puts("Corrupted huffman tree.");
				/* maybe clean already allocated data? */
				return 0;
			}
		}
		
		current->value = i+1;
	}
	
	/*char test[] = { 0x72 };
	'%c'\n", (char)huff_get(test, 7, 1));*/
	
	return 1;
}
 

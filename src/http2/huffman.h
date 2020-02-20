/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains HTTP/2-specific symbols about the Huffman encoding. 
 */
#include <stddef.h>

/**
 * Description:
 *   Decode a string from the source using Huffman decompression.
 *
 * Parameters:
 *   const char *
 *     The octet stream to get the data from.
 *   const size_t
 *     The amount of octets the string should take up. This is very likely to be 
 *     smaller than the string length (since it's decompression).
 *
 * Return Value:
 *   The decompressed, NULL-terminated string. This should be freed.
 */
char *huff_decode(const char *, const size_t);

/**
 * Description:
 *   This function will construct the Huffman tree, using the data defined in
 *   "huffman_table.h".
 *
 * Return Value:
 *   Success status.
 */
int huff_setup(void);

/**
 * Description:
 *   This function will destroy all data allocated by huff_setup.
 */
void huff_destroy(void);

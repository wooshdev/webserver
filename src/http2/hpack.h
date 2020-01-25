/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains symbols of untility functions for the decompression & parsing 
 * of a HPACK blob.
 */
#ifndef HTTP2_HPACK_H
#define HTTP2_HPACK_H

#include <stddef.h>
#include "frame.h"

/**
 * Description:
 *   Duplicate a part of a string.
 *
 * Parameters:
 *   const char *
 *     The source string.
 *   size_t
 *     The amount of characters to be copied 
 *     from the source/original string.
 *
 * Return Value:
 *   The new string. This should be freed.
*/
char *dup_str(const char *, size_t);

/**
 * Description:
 *	 Parses a integer as per RFC 7541 Section 5.1
 *
 * Parameters:
 *	 const char *
 *		 The source to get the octets from.
 *	 size_t *
 *		 The integer to store the amount of
 *		 used octets from the "stream" in.
 *		 This amount is added, not replaced.
 *	 size_t
 * 	   N. This is a value defined by the RFC and 
 *     is the minimum amount of bits this function
 *     should use to parse. Because of the nature
 *     of the standard, hereby you can also calculate
 *     the offset from the first octet. This is 8 - N.
 *
 * Example:
 *   RFC 7541 Section 6.1. Indexed Header Field Representation
 *     0   1   2   3   4   5   6   7
 *   +---+---+---+---+---+---+---+---+
 *   | 1 |        Index (7+)         |
 *   +---+---------------------------+
 *  Herein is the '7+' known as 'size_t N', and the offset 
 *  is 1 = (8 - n) = (8 - 7)
 *
 * Return Value:
 *	 The integer.
 */
size_t parse_int(const char *, size_t *, size_t);


/**
 * Description:
 *	 Parses a header frame as per RFC 7541.
 *
 * Parameters:
 *	 frame_t *
 *     The frame, the source of the header data.
 *
 * Return Value:
 *	 Nothing yet, this will change, though.
 */
void handle_headers(frame_t *);

#endif /* HTTP2_HPACK_H */
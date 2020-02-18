/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef UTILS_IO_H
#define UTILS_IO_H

#include <stddef.h>

#include "secure/tlsutil.h"
/**
 * Description:
 *   This function will read from the source until a specific character 
 *   is encountered, OR the maximum size of bytes read has been reached.
 * 
 * Parameters:
 *   TLS
 *     The TLS source to be read from.
 *   char *
 *     The destination buffer.
 *   char
 *     The 
 *   size_t
 *     The maximum bytes to be read (+1 because of the null-terminator char)
 * 
 * Return value:
 *   -1 maximum read-size reached
 *    0 I/O failure
 *   >0 success, the length of the string, excluding the NULL-terminator
 */
int io_read_until(TLS, char *, char, size_t);

#endif /* UTILS_IO_H */

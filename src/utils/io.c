/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * For information about these functions/symbols, see parser.h
 */
#include "io.h"

#include <stdio.h>
#include <stdlib.h>

int io_read_until(TLS source, char *dest, char until, size_t max) {
	char *buffer = malloc(1);
	buffer[0] = 0;

	size_t pos = 0;
	do {
		if (!tls_read_client(source, buffer, 1)) {
			puts("failed to read from tls_read");
			return -2;
		}

		dest[pos] = buffer[0];

		if (buffer[0] == until) {
			dest[pos] = 0;
			return pos;
		}

	} while (++pos != max);

	dest[pos-1] = 0;
	printf("[I/O] Return value will be: -1\n[I/O] Dest modified: '%s'\n", dest);

	return -1;
}

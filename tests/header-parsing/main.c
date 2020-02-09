/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../../src/http/header_parser.h"

#define T_MILLION 1E6

int main(int argc, const char **argv) {
	if (argc <= 1) {
		fputs("\x1B[31mError: Please add the header value as argument to this program!\n", stderr);
		fprintf(stderr, "For example: %s \"gzip, deflate\"\x1B[0m\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (argc > 2) {
		fputs("\x1B[31mError: It seems that you have entered too many arguments.\nIf your header value contains white spaces, please surround the value with (double) quotation marks!\n", stderr);
		fprintf(stderr, "For example: \x1B[33m%s \x1B[32m\"\x1B[33mgzip, deflate\x1B[32m\"\x1B[0m\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *compression_names[] = { "ERROR", "NONE", "GZIP", "ANY" };
	struct timespec start, end;
	clock_gettime(CLOCK_REALTIME, &start);
	compression_t type = http_parse_accept_encoding(argv[1]);
	clock_gettime(CLOCK_REALTIME, &end);

	double accumulator = (end.tv_sec - start.tv_sec)/1E3 + (end.tv_nsec - start.tv_nsec)/T_MILLION;

	printf("\x1B[34mInput: \x1B[32m%s\x1B[34m\r\nOutput: \x1B[32m%s \x1B[0m(\x1B[32m0x%x\r\n\x1B[34mTime Elapsed: \x1B[32m%f ms\x1B[0m\r\n", argv[1], compression_names[type], type, accumulator);

	return EXIT_SUCCESS;
}

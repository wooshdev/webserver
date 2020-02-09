# Copyright (C) 2019-2020 Tristan
# For conditions of distribution
# and use, see copyright notice in
# the COPYING file

CFLAGS = -O3 -Wall -g
LDFLAGS = -pthread `pkg-config --static --libs openssl`
CC = c89

SUBBINARIES = ../../bin/http/header_parser.so ../../bin/util.o

testbin: main.c $(SUBBINARIES)
	$(CC) $(CFLAGS) -o $@ $< $(SUBBINARIES) $(LDFLAGS)
../../bin/http/header_parser.so: ../../src/http/header_parser.c ../../src/http/header_parser.h
	$(CC) -o $@ -c $(CFLAGS) $<
../../bin/util.o: ../../src/utils/util.c ../../src/utils/util.h
	$(CC) -o $@ -c $(CFLAGS) $<
memtest:
	@echo 'valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=memtest-out.txt ./testbin'

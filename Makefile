# Copyright (C) 2019-2020 Tristan
# For conditions of distribution
# and use, see copyright notice in
# the COPYING file.

OUTPUTFILE = bin/server

#-g
CFLAGS = -O3 -Wall -g
LDFLAGS = -pthread `pkg-config --static --libs openssl`
CC = c89

# unfortunately, because of the 'bin/build.txt' hack we can't use the '$^' macro, because bin/build.txt isn't accepted by ld, maybe a FIXME?
HTTP2BINARIES = bin/http2/constants.so bin/http2/dynamic_table.so bin/http2/frame.so bin/http2/hpack.so bin/http2/huffman.so
SUBBINARIES = bin/server.o bin/config_reader.o bin/config_validation.o bin/file_util.o bin/util.o bin/secure/implopenssl.o bin/io.so bin/http/parser.so bin/http/common.so bin/handling/handlers.so bin/http/http1.so bin/http/http2.so $(HTTP2BINARIES)

$(OUTPUTFILE): src/main.c bin/build.txt $(SUBBINARIES)
	$(CC) $(CFLAGS) -o $@ $< $(SUBBINARIES) $(LDFLAGS)
bin/build.txt: # a hack to create the bin folder only once
	mkdir -p bin/handling
	mkdir -p bin/http
	mkdir -p bin/http2
	mkdir -p bin/secure
	touch bin/build.txt
bin/config_reader.o: src/configuration/reader.c src/configuration/config.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/config_validation.o: src/configuration/validator.c src/configuration/config.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/file_util.o: src/utils/fileutil.c src/utils/fileutil.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/util.o: src/utils/util.c src/utils/util.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/server.o: src/server.c src/server.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/secure/implopenssl.o: src/secure/impl/implopenssl.c src/secure/tlsutil.h bin/file_util.o src/secure/impl/ossl-ocsp.c
	$(CC) -o $@ -c $(CFLAGS) $< $(LDFLAGS)
bin/io.so: src/utils/io.c src/utils/io.h src/secure/tlsutil.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/http/parser.so: src/http/parser.c src/http/parser.h bin/io.so src/utils/io.h 
	$(CC) -o $@ -c $(CFLAGS) $<
bin/http/http1.so: src/http/http1.c src/http/http1.h bin/http/parser.so
	$(CC) -o $@ -c $(CFLAGS) $<
bin/http/http2.so: src/http2/core.c src/http2/core.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/http/common.so: src/http/common.c src/http/common.h bin/io.so src/utils/io.h 
	$(CC) -o $@ -c $(CFLAGS) $<
bin/handling/handlers.so: src/handling/handlers.c src/handling/handlers.h
	$(CC) -o $@ -c $(CFLAGS) $<
# HTTP/2 Binaries
bin/http2/constants.so: src/http2/constants.c src/http2/constants.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/http2/dynamic_table.so: src/http2/dynamic_table.c src/http2/dynamic_table.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/http2/frame.so: src/http2/frame.c src/http2/frame.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/http2/hpack.so: src/http2/hpack.c src/http2/hpack.h bin/http2/dynamic_table.so bin/http2/huffman.so
	$(CC) -o $@ -c $(CFLAGS) $<
bin/http2/huffman.so: src/http2/huffman.c src/http2/huffman.h src/http2/huffman_table.h
	$(CC) -o $@ -c $(CFLAGS) $<
memtest:
	valgrind --leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--verbose \
	--log-file=memtest-out.txt \
	$(OUTPUTFILE)

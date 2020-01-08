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
SUBBINARIES = bin/server.o bin/config_reader.o bin/config_validation.o

$(OUTPUTFILE): src/main.c bin/build.txt $(SUBBINARIES)
	$(CC) $(CFLAGS) -o $@ $< $(SUBBINARIES) $(LDFLAGS)
bin/build.txt: # a hack to create the bin folder only once
	mkdir bin
	touch bin/build.txt
bin/config_reader.o: src/configuration/reader.c src/configuration/config.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/config_validation.o: src/configuration/validator.c src/configuration/config.h
	$(CC) -o $@ -c $(CFLAGS) $<
bin/server.o: src/server.c src/server.h
	$(CC) -o $@ -c $(CFLAGS) $<
memtest:
	valgrind --leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--verbose \
	--log-file=memtest-out.txt \
	$(OUTPUTFILE)

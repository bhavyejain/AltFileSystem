CC=gcc

BIN=./bin
SOURCE=./src

SHELL = /bin/sh
PKGFLAGS = `pkg-config fuse3 --cflags --libs`

CFLAGS = -g -Og -Wall -std=gnu11 $(PKGFLAGS)

$(shell  mkdir -p $(BIN))

filesystem: $(BIN)/altfs 

$(BIN)/altfs: $(SOURCE)/fuse_fs.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf obj/*
	rm -rf ./bin
CC=gcc

SHELL = /bin/sh
PKGFLAGS = `pkg-config fuse --cflags --libs`

CFLAGS = -g -Og -Wall -std=gnu11 $(PKGFLAGS)

altfs: fuse_fs.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf obj/*
CC=gcc

SHELL = /bin/sh
PKGFLAGS = `pkg-config fuse3 --cflags --libs`

CFLAGS = -g -Og -Wall -std=gnu11 $(PKGFLAGS)

altfs: fuse_fs.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf obj/*
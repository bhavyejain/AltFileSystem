CC=gcc

BIN=./bin
SOURCE=./src

SHELL = /bin/sh

CFLAGS = -g -Og -Wall -std=gnu11
DEBUG_FLAGS  = -g -O0 -Wall -std=gnu11

.DELETE_ON_ERROR:

1_create_open_write: ./1_create_open_write.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

2_seek_read_write: ./2_seek_read_write.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

3_delete: ./3_delete.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

4_directory: ./4_directory.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

5_large: ./5_large.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

6_extra_large: ./6_extra_large.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

all: 1_create_open_write 2_seek_read_write 3_delete 4_directory 5_large 6_extra_large

clean:
	rm -rf obj/*
	rm -rf $(BIN)
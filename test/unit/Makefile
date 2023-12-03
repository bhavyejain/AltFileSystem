CC=gcc

BIN=./bin
SOURCE=../../src

SHELL = /bin/sh
PKGFLAGS = `pkg-config fuse3 --cflags --libs`

CFLAGS = -g -Og -I./header -Wall -std=gnu11 $(PKGFLAGS)
DEBUG_FLAGS  = -g -O0 -I./header -Wall -std=gnu11 $(PKGFLAGS)

.DELETE_ON_ERROR:

test_disk_layer: ./test_disk_layer.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

test_superblock_layer: ./test_superblock_layer.c 
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

test_dblock_inode_layer: ./test_dblock_inode_layer.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

test_dblock_inode_freelist: ./test_dblock_inode_freelist.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

test_inode_data_block_ops: ./test_inode_data_block_ops.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

test_inode_cache: ./test_inode_cache.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

test_directory_ops: ./test_directory_ops.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

test_interface_layer: ./test_interface_layer.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

test_interface_layer_disk: ./test_interface_layer.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS) -DDISK_MEMORY

unit_tests: test_disk_layer test_superblock_layer test_dblock_inode_layer test_dblock_inode_freelist test_inode_data_block_ops test_directory_ops test_interface_layer

# ============ CLEAN =============

clean:
	rm -rf obj/*
	rm -rf $(BIN)
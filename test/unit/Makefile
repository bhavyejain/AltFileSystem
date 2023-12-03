CC=gcc

BIN=./bin
SOURCE=../../src

SHELL = /bin/sh
PKGFLAGS = `pkg-config fuse3 --cflags --libs`

CFLAGS = -g -Og -I./header -Wall -std=gnu11 $(PKGFLAGS)
DEBUG_FLAGS  = -g -O0 -I./header -Wall -std=gnu11 $(PKGFLAGS)

.DELETE_ON_ERROR:

01_disk_layer: ./01_disk_layer.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

02_superblock_layer: ./02_superblock_layer.c 
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

03_dblock_inode_freelist: ./03_dblock_inode_freelist.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

04_dblock_inode_layer: ./04_dblock_inode_layer.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

05_inode_data_block_ops: ./05_inode_data_block_ops.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

06_inode_cache: ./06_inode_cache.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

07_directory_ops: ./07_directory_ops.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

08_interface_layer: ./08_interface_layer.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS)

test_interface_layer_disk: ./08_interface_layer.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $(BIN)/$@ $^ $(DEBUG_FLAGS) -DDISK_MEMORY

unit_tests: 01_disk_layer 02_superblock_layer 03_dblock_inode_freelist 04_dblock_inode_layer 05_inode_data_block_ops 06_inode_cache 07_directory_ops 08_interface_layer

# ============ CLEAN =============

clean:
	rm -rf obj/*
	rm -rf $(BIN)
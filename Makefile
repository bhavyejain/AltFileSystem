CC=gcc

BIN=./bin
SOURCE=./src

SHELL = /bin/sh
PKGFLAGS = `pkg-config fuse3 --cflags --libs`

CFLAGS = -g -Og -I./header -Wall -std=gnu11 $(PKGFLAGS)
DEBUG_FLAGS  = -g -O0 -I./header -Wall -std=gnu11 $(PKGFLAGS)

.DELETE_ON_ERROR:

filesystem: $(BIN)/altfs 
filesystem_debug: $(BIN)/altfs_debug

$(BIN)/altfs: $(SOURCE)/fuse_layer.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $@ $^ $(CFLAGS) -DDISK_MEMORY

$(BIN)/altfs_debug: $(SOURCE)/fuse_layer.c
		$(shell mkdir -p $(BIN))
		$(CC) -o $@ $^ $(DEBUG_FLAGS) -DDISK_MEMORY

mkfs: $(SOURCE)/mkfs.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $@ $^ $(CFLAGS) -DDISK_MEMORY

# ============= TESTING =============
TEST=./test
TEST_BIN=./test/bin

UNIT_TEST=./test/unit
UNIT_TEST_BIN=./test/unit/bin

test_disk_layer: $(UNIT_TEST)/test_disk_layer.c
	$(shell  mkdir -p $(UNIT_TEST_BIN))
	$(CC) -o $(UNIT_TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_superblock_layer: $(UNIT_TEST)/test_superblock_layer.c 
	$(shell  mkdir -p $(UNIT_TEST_BIN))
	$(CC) -o $(UNIT_TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_dblock_inode_layer: $(UNIT_TEST)/test_dblock_inode_layer.c
	$(shell  mkdir -p $(UNIT_TEST_BIN))
	$(CC) -o $(UNIT_TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_dblock_inode_freelist: $(UNIT_TEST)/test_dblock_inode_freelist.c
	$(shell  mkdir -p $(UNIT_TEST_BIN))
	$(CC) -o $(UNIT_TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_inode_data_block_ops: $(UNIT_TEST)/test_inode_data_block_ops.c
	$(shell  mkdir -p $(UNIT_TEST_BIN))
	$(CC) -o $(UNIT_TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_inode_cache: $(UNIT_TEST)/test_inode_cache.c
	$(shell  mkdir -p $(UNIT_TEST_BIN))
	$(CC) -o $(UNIT_TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_directory_ops: $(UNIT_TEST)/test_directory_ops.c
	$(shell  mkdir -p $(UNIT_TEST_BIN))
	$(CC) -o $(UNIT_TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_interface_layer: $(UNIT_TEST)/test_interface_layer.c
	$(shell  mkdir -p $(UNIT_TEST_BIN))
	$(CC) -o $(UNIT_TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_interface_layer_disk: $(UNIT_TEST)/test_interface_layer.c
	$(shell  mkdir -p $(UNIT_TEST_BIN))
	$(CC) -o $(UNIT_TEST_BIN)/$@ $^ $(DEBUG_FLAGS) -DDISK_MEMORY

unit_tests: test_disk_layer test_superblock_layer test_dblock_inode_layer test_dblock_inode_freelist test_inode_data_block_ops test_directory_ops test_interface_layer

# ============ CLEAN =============

clean:
	rm -rf obj/*
	rm -rf $(BIN)
	rm -rf $(TEST_BIN)
	rm -rf $(UNIT_TEST_BIN)

clean_tests:
	rm -rf $(TEST_BIN)
	rm -rf $(UNIT_TEST_BIN)
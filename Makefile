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
		$(CC) -o $@ $^ $(DEBUG_FLAGS)

mkfs: $(SOURCE)/mkfs.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $@ $^ $(CFLAGS) -DDISK_MEMORY

# ============= TESTING =============
TEST=./test
TEST_BIN=./test/bin

test_disk_layer: test/test_disk_layer.c
	$(shell  mkdir -p $(TEST_BIN))
	$(CC) -o $(TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_superblock_layer: test/test_superblock_layer.c 
	$(shell  mkdir -p $(TEST_BIN))
	$(CC) -o $(TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_dblock_inode_layer: test/test_dblock_inode_layer.c
	$(shell  mkdir -p $(TEST_BIN))
	$(CC) -o $(TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_dblock_inode_freelist: test/test_dblock_inode_freelist.c
	$(shell  mkdir -p $(TEST_BIN))
	$(CC) -o $(TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_inode_data_block_ops: test/test_inode_data_block_ops.c
	$(shell  mkdir -p $(TEST_BIN))
	$(CC) -o $(TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_directory_ops: test/test_directory_ops.c
	$(shell  mkdir -p $(TEST_BIN))
	$(CC) -o $(TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_interface_layer: test/test_interface_layer.c
	$(shell  mkdir -p $(TEST_BIN))
	$(CC) -o $(TEST_BIN)/$@ $^ $(DEBUG_FLAGS)

test_interface_layer_disk: test/test_interface_layer.c
	$(shell  mkdir -p $(TEST_BIN))
	$(CC) -o $(TEST_BIN)/$@ $^ $(DEBUG_FLAGS) -DDISK_MEMORY

tests: test_disk_layer test_superblock_layer test_dblock_inode_layer test_dblock_inode_freelist test_inode_data_block_ops test_directory_ops test_interface_layer

# ============ CLEAN =============

clean:
	rm -rf obj/*
	rm -rf ./bin
	rm -rf ./test/bin

clean_tests:
	rm -rf ./test/bin
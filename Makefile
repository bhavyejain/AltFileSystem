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

mkaltfs: $(SOURCE)/mkfs.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $@ $^ $(CFLAGS) -DDISK_MEMORY

# ============= TESTING =============
E2E_TES_BIN=./test/e2e/bin
UNIT_TEST_BIN=./test/unit/bin

# ============ CLEAN =============

clean:
	rm -rf obj/*
	rm -rf $(BIN)
	rm -rf $(E2E_TES_BIN)
	rm -rf $(UNIT_TEST_BIN)
	rm -f mkaltfs

clean_tests:
	rm -rf $(E2E_TES_BIN)
	rm -rf $(UNIT_TEST_BIN)

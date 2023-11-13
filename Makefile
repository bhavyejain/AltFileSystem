CC=gcc

BIN=./bin
SOURCE=./src

TEST=./test
TEST_BIN=./test/bin
TESTS = test1 # add more tests here

SHELL = /bin/sh
PKGFLAGS = `pkg-config fuse3 --cflags --libs`

CFLAGS = -g -Og -Wall -std=gnu11 $(PKGFLAGS)
DEBUG_FLAGS  = -g -O0 -Wall -std=gnu11 $(PKGFLAGS)

.DELETE_ON_ERROR:

filesystem: $(BIN)/altfs 
filesystem_debug: $(BIN)/altfs_debug

$(BIN)/altfs: $(SOURCE)/disk_layer.c
	$(shell  mkdir -p $(BIN))
	$(CC) -o $@ $^ $(CFLAGS)

$(BIN)/altfs_debug: $(SOURCE)/disk_layer.c
		$(shell mkdir -p $(BIN))
		$(CC) -o $@ $^ $(DEBUG_FLAGS)

tests: $(TESTS)

$(TESTS): %: $(TEST)/%.c
	$(shell  mkdir -p $(TEST_BIN))
	$(CC) -o $(TEST_BIN)/$@ $^
	
clean:
	rm -rf obj/*
	rm -rf ./bin
	rm -rf ./test/bin

clean_tests:
	rm -rf ./test/bin
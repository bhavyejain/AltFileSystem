#ifndef __DISK_LAYER__
#define __DISK_LAYER__

#define FUSE_USE_VERSION 31

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

#define ALTFS_ALLOC_MEMORY "altfs_alloc_memory"
#define ALTFS_DEALLOC_MEMORY "altfs_dealloc_memory"
#define ALTFS_READ_BLOCK "altfs_read_block"
#define ALTFS_WRITE_BLOCK "altfs_write_block"

#define BLOCK_SIZE ((ssize_t) 4096)
#define BLOCK_COUNT ((ssize_t) (FS_SIZE/BLOCK_SIZE))
#define FS_SIZE ((ssize_t) 104857600) // TODO: put this under if condition once disk implementation is done

// Allocates memory - returns true on success
bool altfs_alloc_memory();
// Frees memory - returns true on success
bool altfs_dealloc_memory();
// Writes from the buffer to a block 
bool altfs_write_block(ssize_t blockid, char *buffer);
// read from the block to the buffer
bool altfs_read_block(ssize_t blockid, char *buffer);
// free memory pointed to by ptr
void altfs_free_memory(void *ptr);

#endif 
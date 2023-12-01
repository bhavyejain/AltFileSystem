#ifndef __DISK_LAYER__
#define __DISK_LAYER__

#include "common_includes.h"

#define ALTFS_ALLOC_MEMORY "altfs_alloc_memory"
#define ALTFS_DEALLOC_MEMORY "altfs_dealloc_memory"
#define ALTFS_READ_BLOCK "altfs_read_block"
#define ALTFS_WRITE_BLOCK "altfs_write_block"

#define DISK_MEMORY 1

#ifdef DISK_MEMORY
    #define DEVICE_NAME "/dev/vdc"
    #define FS_SIZE ((ssize_t) 1073741824) // TODO - Change to 40 GB later
#else
    #define FS_SIZE ((ssize_t) 104857600*5) 
#endif

#define BLOCK_COUNT ((ssize_t) (FS_SIZE/BLOCK_SIZE))

// Allocates memory - returns true on success
bool altfs_alloc_memory();

// Frees memory - returns true on success
bool altfs_dealloc_memory();

// Writes from the buffer to a block 
bool altfs_write_block(ssize_t blockid, char *buffer);

// read from the block to the buffer
bool altfs_read_block(ssize_t blockid, char *buffer);

#endif 

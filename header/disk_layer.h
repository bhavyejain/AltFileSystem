#ifndef __DISK_LAYER__
#define __DISK_LAYER__

#include "common_includes.h"

#define ALTFS_ALLOC_MEMORY "altfs_alloc_memory"
#define ALTFS_DEALLOC_MEMORY "altfs_dealloc_memory"
#define ALTFS_READ_BLOCK "altfs_read_block"
#define ALTFS_WRITE_BLOCK "altfs_write_block"

#ifdef DISK_MEMORY
    #ifndef DEVICE_NAME
    #define DEVICE_NAME "/dev/vdb"
    #endif
    // Currently 10GB TODO - Change to 40 GB later
    #ifndef FS_GB
    #define FS_GB 40
    #endif

    #define FS_SIZE ((ssize_t) 1073741824 * FS_GB)
#else
    // 10M * n 
    #define FS_SIZE ((ssize_t) 104857600*5)
#endif

#define BLOCK_COUNT ((ssize_t) (FS_SIZE/BLOCK_SIZE))

// Allocates memory - returns true on success
bool altfs_alloc_memory(bool erase);

// Frees memory - returns true on success
bool altfs_dealloc_memory();

// Writes from the buffer to a block 
bool altfs_write_block(ssize_t blockid, char *buffer);

// read from the block to the buffer
bool altfs_read_block(ssize_t blockid, char *buffer);

// Open the mounted volume
bool altfs_open_volume();

#endif 

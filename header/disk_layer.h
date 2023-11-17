#ifndef __DISK_LAYER__
#define __DISK_LAYER__

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

#define ALTFS_ALLOC_MEMORY "altfs_alloc_memory"
#define ALTFS_DEALLOC_MEMORY "altfs_dealloc_memory"
#define ALTFS_READ_MEMORY "altfs_read_memory"
#define ALTFS_WRITE_MEMORY "altfs_write_memory"

#define BLOCK_SIZE ((ssize_t) 4096)
#define BLOCK_COUNT ((ssize_t) (FS_SIZE/BLOCK_SIZE))
#define FS_SIZE ((ssize_t) 104857600) // TODO: put this under if condition once disk implementation is done

// Allocates memory - returns true on success
bool altfs_alloc_memory();
// Frees memory - returns true on success
bool altfs_dealloc_memory();

#endif 
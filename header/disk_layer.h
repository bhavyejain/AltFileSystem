#ifndef __DISK_LAYER__
#define __DISK_LAYER__

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

// Allocates memory - returns true on success
bool altfs_alloc_memory();
// Frees memory - returns true on success
bool altfs_dealloc_memory();

#endif 
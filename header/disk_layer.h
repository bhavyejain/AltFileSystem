#ifndef _DISK_LAYER_HEADER_
#define _DISK_LAYER_HEADER_

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

// Allocates memory - returns true on success
char* altfs_alloc_memory();
// Frees memory - returns true on success
bool altfs_dealloc_memory();

#endif 
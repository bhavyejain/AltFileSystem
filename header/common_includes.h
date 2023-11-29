#ifndef __COMMON_INCLUDES__
#define __COMMON_INCLUDES__

#include <fuse.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE ((ssize_t) 4096)

void altfs_free_memory(void *ptr)
{
    if(ptr != NULL)
    {
        free(ptr);
        ptr = NULL;
    }
}

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 31
#endif
#endif
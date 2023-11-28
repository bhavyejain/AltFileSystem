#ifndef __COMMON_INCLUDES__
#define __COMMON_INCLUDES__

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 31
#endif

#include <fuse.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void altfs_free_memory(void *ptr)
{
    if(ptr != NULL)
        free(ptr);
}

#endif
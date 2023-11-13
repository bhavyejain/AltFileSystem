#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>
//#include<fuse_log.h>
#include "../header/disk_layer.h"

#define ALTFS_ALLOC_MEMORY "altfs_alloc_memory"
#define ALTFS_DEALLOC_MEMORY "altfs_dealloc_memory"

char *fs_memory;

bool altfs_alloc_memory()
{
    fprintf(stderr, "%s Allocating memory\n", ALTFS_ALLOC_MEMORY);
    // TODO: Check that it works for all data types in files
    //fs_memory = (char*)malloc(1048576); // allocate 1 MB
    // TODO: check if calloc works same as malloc+memset
    // TODO: Check why this should be cast to char* and not void*
    fs_memory = (char*)calloc(1,1048576);
    if (!fs_memory)
    {
        fprintf(stderr, "%s Error allocating memory\n", ALTFS_ALLOC_MEMORY);
        return false;
    }
    //memset(fs_memory,0,1048576);
    //fuse_log(FUSE_LOG_DEBUG, "Inside atlfs alloc memory");
    fprintf(stderr, "%s Allocated memory for FS at %p\n", ALTFS_ALLOC_MEMORY, &fs_memory);
    return true;
}

bool altfs_dealloc_memory()
{
    fprintf(stderr, "%s Deallocating memory\n",ALTFS_DEALLOC_MEMORY);
    if (!fs_memory)
        free(fs_memory);
    else
    {
        fprintf(stderr,"%s No pointer to deallocate memory\n",ALTFS_DEALLOC_MEMORY);
        return false;
    }
    fprintf(stderr,"%s Deallocated memory\n",ALTFS_DEALLOC_MEMORY);
    return true;
}


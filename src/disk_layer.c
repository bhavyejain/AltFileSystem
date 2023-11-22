#define FUSE_USE_VERSION 31

#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>
#include<fuse.h>
#include<stdarg.h>
#include<string.h>
#include "../header/disk_layer.h"

// TODO: change this while moving to disk 
static char *mem_ptr;

bool altfs_alloc_memory()
{
    // TODO: Change this when moving to disk
    fuse_log(FUSE_LOG_DEBUG, "%s Allocating memory\n",ALTFS_ALLOC_MEMORY);
    // TODO: Check that it works for all data types in files
    //mem_ptr = (char*)malloc(1048576); // allocate 1 MB
    // TODO: check if calloc works same as malloc+memset
    // TODO: Check why this should be cast to char* and not void*
    mem_ptr = (char*)calloc(1,1048576);
    if (!mem_ptr)
    {
        fuse_log(FUSE_LOG_ERR, "%s Error allocating memory\n",ALTFS_ALLOC_MEMORY);
        return false;
    }
    //memset(mem_ptr,0,1048576);
    fuse_log(FUSE_LOG_DEBUG, "%s Allocated memory for FS at %p\n", ALTFS_ALLOC_MEMORY, &mem_ptr);
    return true;
}

bool altfs_dealloc_memory()
{
    // TODO: Change this when moving to disk
    fuse_log(FUSE_LOG_DEBUG, "%s Deallocating memory\n",ALTFS_DEALLOC_MEMORY);
    if (!mem_ptr)
        free(mem_ptr);
    else
    {
        fuse_log(FUSE_LOG_ERR, "%s No pointer to deallocate memory\n",ALTFS_DEALLOC_MEMORY);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Deallocated memory\n",ALTFS_DEALLOC_MEMORY);
    return true;
}

bool isBlockOutOfRange(ssize_t blockid)
{
    return (blockid < 0 || blockid >= BLOCK_COUNT);
}

bool altfs_read_block(ssize_t blockid, char *buffer)
{
    if (!buffer)
        return false;
    if (isBlockOutOfRange(blockid))
    {
        fuse_log(FUSE_LOG_ERR, "%s Error allocating memory while initializing FS\n",ALTFS_READ_BLOCK);
        return false;
    }
    //TODO: Add code to read from disk here
    ssize_t offset = blockid*BLOCK_SIZE;
    // TODO: Check if there is a possibility that buffer is smaller than required size
    memcpy(buffer, mem_ptr+offset, BLOCK_SIZE);
    return true;
}

bool altfs_write_block(ssize_t blockid, char *buffer)
{
    if (!buffer)
        return false;
    if (isBlockOutOfRange(blockid))
    {
        fuse_log(FUSE_LOG_ERR, "%s Blockid is out of range\n",ALTFS_WRITE_BLOCK);
        return false;
    }
    ssize_t offset = BLOCK_SIZE*blockid;
    fuse_log(FUSE_LOG_DEBUG,"write block: %ld buffer: %s\n", blockid, *buffer);
    fuse_log(FUSE_LOG_DEBUG,"pointer: %p ptr val: %s\n",mem_ptr+offset, *(mem_ptr+offset));
    memcpy(mem_ptr+offset, buffer, BLOCK_SIZE);
    return true;
}
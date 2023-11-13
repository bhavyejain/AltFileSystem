#include<stdlib.h>
#include<stdio.h>
#include "../src/disk_layer.c"

#define DISK_LAYER_TEST "altfs_disklayer_test"
#define SUCCESS "Success: "
#define FAILED "Failed: "

int main(int argc, char *argv[])
{
    // Test1 : Allocate memory
    bool altfs_alloc = altfs_alloc_memory();
    if (!altfs_alloc)
    {
        fprintf(stderr, "%s Test1: %s Failed to allocate memory\n",DISK_LAYER_TEST,FAILED);
        return -1;
    }
    fprintf(stderr, "%s Test1: Success: Allocated memory\n",DISK_LAYER_TEST,SUCCESS);

    bool altfs_dealloc = altfs_dealloc_memory();
    if (!altfs_dealloc)
    {
        fprintf(stderr, "%s Test2: %s Failed to deallocate memory\n",DISK_LAYER_TEST,FAILED);
        return -1;
    }
    fprintf(stderr, "%s Test2: %s Deallocated memory\n",DISK_LAYER_TEST,SUCCESS);

    return 0;
}
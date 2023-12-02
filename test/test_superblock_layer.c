#include<stdio.h>
#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"

#include "test_helpers.c"

#define SUPERBLOCK_LAYER_TEST "altfs_superblock_layer_test"
#define SUCCESS "Success: "
#define FAILED "Failed: "

int main(int argc, char *argv[])
{
    // Test1 : Call makefs
    print_constants();
    printf("\n========== TESTING SUPERBLOCK LAYER ==========\n\n");
    #ifndef DISK_LAYER
    bool makefs = altfs_makefs();
    if (!makefs)
    {
        fprintf(stderr, "%s Test1: %s Failed during makefs\n",SUPERBLOCK_LAYER_TEST,FAILED);
        return -1;
    }
    fprintf(stderr, "%s Test1: %s Makefs completed\n",SUPERBLOCK_LAYER_TEST,SUCCESS);
    #endif

    char *buffer = (char*)malloc(BLOCK_SIZE);
    // read block 0 = superblock
    if (!altfs_read_block(0, buffer))
    {
        fprintf(stderr, "%s : Failed to read block 0 for superblock\n", SUPERBLOCK_LAYER_TEST);
        return -1;
    }
    struct superblock *superblockObj = (struct superblock*)buffer;

    // Test2 : Verify superblock initialization
    print_superblock(superblockObj);
    fprintf(stdout, "%s Test2: %s Printed superblock contents\n",SUPERBLOCK_LAYER_TEST, SUCCESS);

    // Test3 : Verify freelist by printing first free block
    int nextfreeblock = print_freelist(superblockObj->s_freelist_head);
    fprintf(stdout, "%s Test3: %s Printed freelist contents\n",SUPERBLOCK_LAYER_TEST, SUCCESS);
    
    // Test4 : Verify first 10 free blocks
    for(int i=1;i<10;i++)
    {
        if (nextfreeblock > -1)
            nextfreeblock = print_freelist(nextfreeblock);
    }
    fprintf(stdout, "%s Test4: %s Printed first 10 freelist block contents\n",SUPERBLOCK_LAYER_TEST, SUCCESS);

    printf("\n========== RUNNING TESTS COMPLETE ==========\n\n");
    return 0;
}
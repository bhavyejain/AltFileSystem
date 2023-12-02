#include<stdlib.h>
#include<stdio.h>

#include "../../src/disk_layer.c"

#define DISK_LAYER_TEST "altfs_disklayer_test"
#define SUCCESS "Success: "
#define FAILED "Failed: "

int main(int argc, char *argv[])
{
    // Test1 : Allocate memory
    bool altfs_alloc = altfs_alloc_memory();
    if (!altfs_alloc)
    {
        printf("%s Test1: %s Failed to allocate memory\n",DISK_LAYER_TEST,FAILED);
        return -1;
    }
    printf("%s Test1: %s Allocated memory\n",DISK_LAYER_TEST,SUCCESS);

    char* buff = (char *)malloc(BLOCK_SIZE); 

    // Test2 : Read from invalid block
    if (altfs_read_block(-1, buff))
    {
        printf("%s Test2: %s Failed to stop reading from invalid block\n",DISK_LAYER_TEST,FAILED);
        return -1;
    }
    printf("%s Test2: %s Read from invalid block\n",DISK_LAYER_TEST,SUCCESS);

    //Test3: Read from valid block 5
    if (!altfs_read_block(5, buff))
    {
        printf("%s Test3: %s Failed to read from valid block\n",DISK_LAYER_TEST,FAILED);
        return -1;
    }
    printf("%s Test3: %s Read from valid block\n",DISK_LAYER_TEST,SUCCESS);

    // Test4: Check values in block - should be all zeroes
    for(ssize_t i = 0; i < BLOCK_SIZE; i++)
    {
        if (buff[i] != 0)
        {
            printf("%s Test4: %s Not all values of block are zero\n",DISK_LAYER_TEST,FAILED);
            return -1;
        }
    }
    printf("%s Test4: %s All values of block are zero\n",DISK_LAYER_TEST,SUCCESS);

    // Test5: write to invalid block
    if (altfs_write_block(-1,buff))
    {
        printf("%s Test5: %s Failed to stop writing to invalid block\n",DISK_LAYER_TEST,FAILED);
        return -1;
    }
    printf("%s Test5: %s Write to invalid block\n",DISK_LAYER_TEST,SUCCESS);

    // Test6: Write to valid block
    char *teststr = "This is a test string.";
    memcpy(buff,teststr,strlen(teststr));
    if (!altfs_write_block(10,buff))
    {
        printf("%s Test6: %s Failed to write to valid block\n",DISK_LAYER_TEST,FAILED);
        return -1;
    }
    printf("%s Test6: %s Write to valid block\n",DISK_LAYER_TEST,SUCCESS);

    // Test7 : Read from block 10
    if (!altfs_read_block(10, buff))
    {
        printf("%s Test7: %s Failed to read from valid block\n",DISK_LAYER_TEST,FAILED);
        return -1;
    }
    if (strcmp(buff, teststr) == 0)
        printf("%s Test7: %s contents are same as teststr\n", DISK_LAYER_TEST, SUCCESS);
    else
    {
        printf("%s Test7: %s contents are not the as teststr\n", DISK_LAYER_TEST, FAILED);
        return -1;
    }
    printf("%s Test7: %s Read from valid block\n",DISK_LAYER_TEST,SUCCESS);

    // Test8 : Write to all blocks and read
    for(ssize_t i=0; i<BLOCK_COUNT; i++)
    {
        memcpy(buff, teststr, strlen(teststr));
        if(!altfs_write_block(i, buff))
        {
            printf("%s Test8: Write to block %ld failed\n",DISK_LAYER_TEST, i);
            return -1;
        }
        if(!altfs_read_block(i, buff))
        {
            printf("%s Test8: Read from block %ld failed\n",DISK_LAYER_TEST, i);
            return -1;
        }
        if(strcmp(buff, teststr) != 0)
        {
            printf("%s Test8: String compare for block %ld failed\n",DISK_LAYER_TEST, i);
        }
    }
    printf("%s Test8: Write, read and data compare for all blocks passed\n",DISK_LAYER_TEST);

    /*bool altfs_dealloc = altfs_dealloc_memory();
    if (!altfs_dealloc)
    {
        fprintf(stderr, "%s Test2: %s Failed to deallocate memory\n",DISK_LAYER_TEST,FAILED);
        return -1;
    }
    fprintf(stderr, "%s Test2: %s Deallocated memory\n",DISK_LAYER_TEST,SUCCESS);*/
    printf("\n=========== ALL TESTS PASSED! ===========\n\n");

    return 0;
}
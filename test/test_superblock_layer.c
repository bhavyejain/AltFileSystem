#include<stdio.h>
#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"

#define SUPERBLOCK_LAYER_TEST "altfs_superblock_layer_test"
#define SUCCESS "Success: "
#define FAILED "Failed: "

void print_freelist(struct superblock *superblockObj)
{   
    ssize_t freelist_start_blocknum = superblockObj->s_freelist_head;
    printf("\n******************** FREELIST ********************\n");
    char buff[BLOCK_SIZE];
    ssize_t offset = 0;
    for(ssize_t i = 0; i < NUM_OF_FREE_LIST_BLOCKS; i++)
    {
        printf("Next free list block: %ld\n", freelist_start_blocknum);
        ssize_t currblock = freelist_start_blocknum;
        // buffer has contents of 1 free list block
        memset(buff, 0, BLOCK_SIZE);
        if (!altfs_read_block(currblock, buff))
        {
            fuse_log(FUSE_LOG_ERR, "Print freelist: Error reading contents of free list block number: %ld\n",currblock);
            return;
        }

        for(ssize_t j = 1; j < NUM_OF_ADDRESSES_PER_BLOCK; j++)
        {
            currblock += 1;
            offset += ADDRESS_SIZE;
            char *blockcontents = (char*)(*(buffer+offset));
            printf("block num: %ld block address: %p block contents: %s\n", currblock, *(buffer+offset), blockcontents);
        }
    }
    printf("\n******************** FREELIST ********************\n");
}

void print_superblock(struct superblock *superblockObj)
{
    /*char *buffer = (char*)malloc(BLOCK_SIZE);
    // read block 0 = superblock
    if (!altfs_read_block(0, buffer))
    {
        fprintf(stderr, "%s : Failed to read block 0 for superblock\n", SUPERBLOCK_LAYER_TEST);
        return;
    }
    struct superblock *superblockObj = (struct superblock*)buffer;*/
    printf("\n******************** SUPERBLOCK ********************\n");
    printf("\nNUM OF INODES: %ld \n NEXT AVAILABLE INODE: %ld \n FREE LIST HEAD: %ld \n INODES PER BLOCK: %ld \n", superblockObj->s_inodes_count, superblockObj->s_first_ino, superblockObj->s_freelist_head, superblockObj->s_num_of_inodes_per_block);
    printf("\n******************** SUPERBLOCK ********************\n");
    return;
}

void print_constants()
{
    printf("\n******************** CONSTANTS ********************\n");
    printf("\n BLOCK COUNT: %ld \n NUM_OF_DATA_BLOCKS: %ld \n NUM OF DIRECT BLOCKS: %ld \n BLOCK SIZE: %ld \n INODES PER BLOCK: %ld\n INODE BLOCK COUNT: %ld\n",BLOCK_COUNT, NUM_OF_DATA_BLOCKS, NUM_OF_DIRECT_BLOCKS, BLOCK_SIZE, (BLOCK_SIZE) / sizeof(struct inode), INODE_BLOCK_COUNT);
    printf("\n******************** CONSTANTS ********************\n");
}

int main(int argc, char *argv[])
{
    // Test1 : Call makefs
    print_constants();
    bool makefs = altfs_makefs();
    if (!makefs)
    {
        fprintf(stderr, "%s Test1: %s Failed during makefs\n",SUPERBLOCK_LAYER_TEST,FAILED);
        return -1;
    }
    fprintf(stderr, "%s Test1: %s Makefs completed\n",SUPERBLOCK_LAYER_TEST,SUCCESS);

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

    // Test3 : 
    return 0;
}
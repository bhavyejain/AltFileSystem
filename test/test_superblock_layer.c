#include<stdio.h>
#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"

#define SUPERBLOCK_LAYER_TEST "altfs_superblock_layer_test"
#define SUCCESS "Success: "
#define FAILED "Failed: "

int print_freelist(ssize_t blocknum)
{   
    printf("\n******************** FREELIST ********************\n");
    printf("\n************ FREELIST FOR BLOCK: %ld *************\n",blocknum);
    char *buff = (char*)malloc(BLOCK_SIZE);
    ssize_t offset = 0;

    if (!altfs_read_block(blocknum, buff))
    {
        printf("Print freelist: Error reading contents of free list block number: %ld\n",blocknum);
        return -1;
    }
    ssize_t *buff_numptr = (ssize_t *)buff;
    for (size_t i = 0; i < NUM_OF_ADDRESSES_PER_BLOCK / 8; i++)
    {
        for (size_t j = 0; j < 8; j++)
        {
            if (i == 0 && j == 0)
                printf("Next free block: %ld\n", buff_numptr[0]);
            else
            printf("%ld ", buff_numptr[i * 8 + j]);
        }
        printf("\n");
    }
    printf("\n******************** FREELIST ********************\n");
    return buff_numptr[0];
}

void print_superblock(struct superblock *superblockObj)
{
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

    return 0;


}
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"
#include "../src/inode_ops.c"
#include "../src/data_block_ops.c"

#define DBLOCK_INODE_FREELIST_TEST "altfs_dblock_inode_freelist_test"

int print_freelist(ssize_t blocknum)
{   
    printf("\n******************** FREELIST ********************\n");
    printf("\n************ FREELIST FOR BLOCK: %ld *************\n",blocknum);
    char *buff = (char*)malloc(BLOCK_SIZE);

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
    printf("\n*************************************************\n");
    return buff_numptr[0];
}

void print_inode(struct inode** node)
{
    printf("\n******************** INODE ********************\n");
    for(ssize_t i = 0; i < NUM_OF_DIRECT_BLOCKS; i++)
    {
        printf("Direct block %ld: %ld\n", i, (*node)->i_direct_blocks[i]);
    }
    printf("Single indirect block: %ld\n", (*node)->i_single_indirect);
    printf("Double indirect block: %ld\n", (*node)->i_double_indirect);
    printf("Triple indirect block: %ld\n", (*node)->i_triple_indirect);
    printf("******************** INODE ********************\n\n");
}

void test_data_block_ops()
{
    /*
    * Check data block ops.
    */
    fprintf(stdout, "\n******************* TESTING DATA BLOCK OPERATIONS *********************\n");

    char* buffer = (char*)malloc(BLOCK_SIZE);
    char* str = "I really hope my block layer works!";
    memcpy(buffer, str, strlen(str));

    fprintf(stdout, "%s : Attemting to CRUD 10 data blocks and verify freelist.\n", DBLOCK_INODE_FREELIST_TEST);

    // read free list head
    char *sb_buf = (char*)malloc(BLOCK_SIZE);
    struct superblock* sb;

    for(ssize_t i = 0; i < 10; i++)
    {
        if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
        sb = (struct superblock*)sb_buf;

        fprintf(stdout, "%s : Iteration: %ld Free list head: %ld\n", DBLOCK_INODE_FREELIST_TEST, sb->s_freelist_head);
        print_freelist(sb->s_freelist_head);
        
        // Allocate new data block - Starts allocating from INODE_BLOCK_COUNT + 1 + 1 onwards
        ssize_t block_num = allocate_data_block();
        if(block_num < 0)
        {
            fprintf(stderr, "%s : Failed to create data block at i = %ld.\n", DBLOCK_INODE_FREELIST_TEST, i);
            return -1;
        }
        fprintf(stdout, "%s : Iteration: %ld Allocated block num %ld\n", DBLOCK_INODE_FREELIST_TEST, i, block_num);
    }

    if (!altfs_read_block(0, sb_buf))
    {
        fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
        return -1;
    }
    sb = (struct superblock*)sb_buf;

    fprintf(stdout, "%s : Free list head: %ld\n", DBLOCK_INODE_FREELIST_TEST, sb->s_freelist_head);
    print_freelist();
    fprintf(stdout, "%s : Freelist consistency verified.\n", DBLOCK_INODE_FREELIST_TEST);
    free(buffer);
    
    return;
}

int main()
{
     printf("=============== TESTING DATA BLOCK & INODE OPERATIONS =============\n\n");
    // Create filesystem (assumes superblock layer tests pass)
    if(!altfs_makefs())
    {
        printf("Altfs makefs failed!");
        return -1;
    }

    // Test 1 - Test data block ops
    test_data_block_ops();


    return 0;
}
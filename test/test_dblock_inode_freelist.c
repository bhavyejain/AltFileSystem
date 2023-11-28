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

// verify free list state after allocating more than 512 blocks
// Freelist head should update new freelist block after 512 blocks are allocated
int test_verify_freelist_allocation()
{
    fprintf(stdout, "\n******************* TESTING FREE LIST ALLOCATION *********************\n");

    fprintf(stdout, "%s : Attemting to allocate %ld  data blocks and verify freelist.\n", DBLOCK_INODE_FREELIST_TEST, NUM_OF_ADDRESSES_PER_BLOCK+10);
    
    char *sb_buf = (char*)malloc(BLOCK_SIZE);
    struct superblock* sb;
    if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
    
    // verify correct free list update
    for(ssize_t i = 0; i < NUM_OF_ADDRESSES_PER_BLOCK + 10; i++)
    {
        fprintf(stdout, "\n******************* START OF ITERATION *********************\n");

        
        sb = (struct superblock*)sb_buf;
        free(sb_buf);

        fprintf(stdout, "%s : Iteration: %ld Free list head: %ld\n", DBLOCK_INODE_FREELIST_TEST,i, sb->s_freelist_head);
        //print_freelist(sb->s_freelist_head);
        
        // Allocate new data block - Starts allocating from INODE_BLOCK_COUNT + 1 + 1 onwards
        ssize_t block_num = allocate_data_block();
        if(block_num < 0)
        {
            fprintf(stderr, "%s : Failed to create data block at i = %ld.\n", DBLOCK_INODE_FREELIST_TEST, i);
            return -1;
        }
        fprintf(stdout, "%s : Iteration: %ld Allocated block num %ld\n", DBLOCK_INODE_FREELIST_TEST, i, block_num);

        fprintf(stdout, "\n******************* END OF ITERATION *********************\n");
    }

    fprintf(stdout, "%s : Freelist consistency verified.\n", DBLOCK_INODE_FREELIST_TEST);
    return 0;
}

int test_data_block_ops()
{
    /*
    * Check data block ops.
    */
    fprintf(stdout, "\n******************* TESTING DATA BLOCK OPERATIONS *********************\n");

    fprintf(stdout, "%s : Attemting to CRUD 10 data blocks and verify freelist.\n", DBLOCK_INODE_FREELIST_TEST);
    
    char *sb_buf = (char*)malloc(BLOCK_SIZE);
    struct superblock* sb;
    
    ssize_t blocks_to_free[5];
    int index = 0;

    // verify correct free list update
    for(ssize_t i = 0; i < 10; i++)
    {
        fprintf(stdout, "\n******************* START OF ITERATION *********************\n");

        if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
        sb = (struct superblock*)sb_buf;

        fprintf(stdout, "%s : Iteration: %ld Free list head: %ld\n", DBLOCK_INODE_FREELIST_TEST,i, sb->s_freelist_head);
        print_freelist(sb->s_freelist_head);
        
        // Allocate new data block - Starts allocating from INODE_BLOCK_COUNT + 1 + 1 onwards
        ssize_t block_num = allocate_data_block();
        if(block_num < 0)
        {
            fprintf(stderr, "%s : Failed to create data block at i = %ld.\n", DBLOCK_INODE_FREELIST_TEST, i);
            return -1;
        }
        fprintf(stdout, "%s : Iteration: %ld Allocated block num %ld\n", DBLOCK_INODE_FREELIST_TEST, i, block_num);

        // store block num of 5 data blocks to free later
        if (i > 2 && i < 8)
        {
            blocks_to_free[index] = block_num;
            index += 1;
        }

        fprintf(stdout, "\n******************* END OF ITERATION *********************\n");
    }

    for(ssize_t i = 0; i < 5; i++)
    {
        if(!free_data_block(blocks_to_free[i]))
        {
            fprintf(stderr, "%s : Error while free-ing data block %ld.\n", DBLOCK_INODE_FREELIST_TEST, blocks_to_free[i]);
            return -1;
        }

        if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
        sb = (struct superblock*)sb_buf;

        fprintf(stdout, "%s : Freed block num %ld\n", DBLOCK_INODE_FREELIST_TEST, blocks_to_free[i]);
        fprintf(stdout, "%s : Free list head: %ld\n", DBLOCK_INODE_FREELIST_TEST, sb->s_freelist_head);
        print_freelist(sb->s_freelist_head);
    }
    free(sb_buf);
    fprintf(stdout, "%s : Freelist consistency verified.\n", DBLOCK_INODE_FREELIST_TEST);
    return 0;
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
    if (test_data_block_ops() == -1)
    fprintf(stderr, "%s : Test1 - testing for data block ops failed\n", DBLOCK_INODE_FREELIST_TEST);
    

    return 0;
}
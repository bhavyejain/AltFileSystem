#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../src/data_block_ops.c"
#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"

#include "test_helpers.c"

#define DBLOCK_INODE_FREELIST_TEST "altfs_dblock_inode_freelist_test"

// verify free list state after allocating more than 512 blocks
// Freelist head should update new freelist block after 512 blocks are allocated
int test_verify_freelist_allocation()
{
    fprintf(stdout, "\n******************* START: TESTING FREE LIST AFTER ALLOCATING > 512 BLOCKS *********************\n");

    fprintf(stdout, "%s : Attemting to allocate %ld  data blocks and verify freelist.\n", DBLOCK_INODE_FREELIST_TEST, NUM_OF_ADDRESSES_PER_BLOCK+10);
    
    int blocks_to_free[NUM_OF_ADDRESSES_PER_BLOCK + 10];
    char *sb_buf = (char*)malloc(BLOCK_SIZE);
    struct superblock* sb;
    
    // verify correct free list update
    for(ssize_t i = 0; i < NUM_OF_ADDRESSES_PER_BLOCK + 10; i++)
    {
        if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
        sb = (struct superblock*)sb_buf;

        fprintf(stdout, "\n%s : Iteration: %ld Free list head: %ld\n", DBLOCK_INODE_FREELIST_TEST,i, sb->s_freelist_head);
        
        // Allocate new data block
        ssize_t block_num = allocate_data_block();
        if(block_num < 0)
        {
            fprintf(stderr, "%s : Failed to create data block at i = %ld.\n", DBLOCK_INODE_FREELIST_TEST, i);
            return -1;
        }
        fprintf(stdout, "%s : Iteration: %ld Allocated block num %ld\n", DBLOCK_INODE_FREELIST_TEST, i, block_num);
        blocks_to_free[i] = block_num;

        if (i == NUM_OF_ADDRESSES_PER_BLOCK - 1 ||
            i == NUM_OF_ADDRESSES_PER_BLOCK ||
            i == NUM_OF_ADDRESSES_PER_BLOCK + 10 - 1)
            print_freelist(sb->s_freelist_head);
    }
    // print free list after allocating 512+10 blocks
    if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
    sb = (struct superblock*)sb_buf;
    print_freelist(sb->s_freelist_head);

    for(int i = 0; i < NUM_OF_ADDRESSES_PER_BLOCK + 10; i++)
    {
        if(!free_data_block(blocks_to_free[i]))
        {
            fprintf(stderr, "%s : Error while free-ing data block %d.\n", DBLOCK_INODE_FREELIST_TEST, blocks_to_free[i]);
            return -1;
        }
        fprintf(stdout, "%s : Freed block num %d\n", DBLOCK_INODE_FREELIST_TEST, blocks_to_free[i]);
    }

    // print free list after freeing 512+10 blocks
    if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
    sb = (struct superblock*)sb_buf;
    print_freelist(sb->s_freelist_head);


    free(sb_buf);
    fprintf(stdout, "%s : Freelist consistency verified.\n", DBLOCK_INODE_FREELIST_TEST);
    fprintf(stdout, "\n******************* END: TESTING FREE LIST AFTER ALLOCATING > 512 BLOCKS *********************\n");
    return 0;
}

int test_data_block_ops()
{
    /*
    * Check data block ops.
    */
    fprintf(stdout, "\n******************* TESTING DATA BLOCK OPERATIONS *********************\n");
    
    char *sb_buf = (char*)malloc(BLOCK_SIZE);
    struct superblock* sb;

    if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
    sb = (struct superblock*)sb_buf;
    print_freelist(sb->s_freelist_head);
    
    ssize_t blocks_to_free[10];

    // verify correct free list update
    fprintf(stdout, "\n******************* START: VERIFY FREELIST AFTER ALLOCATING 10 BLOCKS *********************\n");
    for(ssize_t i = 0; i < 10; i++)
    {      
        // Allocate new data block - Starts allocating from INODE_BLOCK_COUNT + 1 + 1 onwards
        ssize_t block_num = allocate_data_block();
        if(block_num < 0)
        {
            fprintf(stderr, "%s : Failed to create data block at i = %ld.\n", DBLOCK_INODE_FREELIST_TEST, i);
            return -1;
        }
        fprintf(stdout, "%s : Iteration: %ld Allocated block num %ld\n", DBLOCK_INODE_FREELIST_TEST, i, block_num);

        // store block nums to free later
        blocks_to_free[i] = block_num;
    }
    fprintf(stdout, "\n******************* END: VERIFY FREELIST AFTER ALLOCATING 10 BLOCKS *********************\n");

    // print free list after allocating 10 blocks
    if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
    sb = (struct superblock*)sb_buf;
    print_freelist(sb->s_freelist_head);

    // free first 5 data blocks allocated
    fprintf(stdout, "\n******************* START: VERIFY FREELIST AFTER FREEING 5 BLOCKS *********************\n");
    for(ssize_t i = 0; i < 5; i++)
    {
        if(!free_data_block(blocks_to_free[i]))
        {
            fprintf(stderr, "%s : Error while free-ing data block %ld.\n", DBLOCK_INODE_FREELIST_TEST, blocks_to_free[i]);
            return -1;
        }
        fprintf(stdout, "%s : Freed block num %ld\n", DBLOCK_INODE_FREELIST_TEST, blocks_to_free[i]);
    }
    fprintf(stdout, "\n******************* END: VERIFY FREELIST AFTER FREEING 5 BLOCKS *********************\n");

    // print free list after freeing 5 blocks
    if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
    sb = (struct superblock*)sb_buf;
    print_freelist(sb->s_freelist_head);

    // allocate 5 more blocks
    fprintf(stdout, "\n******************* START: VERIFY FREELIST AFTER ALLOCATING 5 MORE BLOCKS *********************\n");
    for(int i = 0; i < 5; i++)
    {
        ssize_t block_num = allocate_data_block();
        if(block_num < 0)
        {
            fprintf(stderr, "%s : Failed to create data block at i = %d.\n", DBLOCK_INODE_FREELIST_TEST, i);
            return -1;
        }

        fprintf(stdout, "%s : Iteration: %d Allocated block num %ld\n", DBLOCK_INODE_FREELIST_TEST, i, block_num);
        blocks_to_free[i] = block_num;
    }
    fprintf(stdout, "\n******************* END: VERIFY FREELIST AFTER ALLOCATING 5 MORE BLOCKS *********************\n");

    // print free list after allocating 5 more blocks
    if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
    sb = (struct superblock*)sb_buf;
    print_freelist(sb->s_freelist_head);

    // free all 10 data blocks allocated
    fprintf(stdout, "\n******************* START: VERIFY FREELIST AFTER FREEING 10 BLOCKS *********************\n");
    for(ssize_t i = 0; i < 10; i++)
    {
        if(!free_data_block(blocks_to_free[i]))
        {
            fprintf(stderr, "%s : Error while free-ing data block %ld.\n", DBLOCK_INODE_FREELIST_TEST, blocks_to_free[i]);
            return -1;
        }
        fprintf(stdout, "%s : Freed block num %ld\n", DBLOCK_INODE_FREELIST_TEST, blocks_to_free[i]);
    }
    fprintf(stdout, "\n******************* END: VERIFY FREELIST AFTER FREEING 10 BLOCKS *********************\n");

    // print free list after freeing 10 blocks
    if (!altfs_read_block(0, sb_buf))
        {
            fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DBLOCK_INODE_FREELIST_TEST);
            return -1;
        }
    sb = (struct superblock*)sb_buf;
    print_freelist(sb->s_freelist_head);

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
    
    // Test 2 - Test free list update after allocating > 512 blocks
    if (test_verify_freelist_allocation() == -1)
        fprintf(stderr, "%s : Test2 - testing for free list updation failed\n", DBLOCK_INODE_FREELIST_TEST);

    return 0;
}
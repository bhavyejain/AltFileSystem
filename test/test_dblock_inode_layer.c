#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"
#include "../src/inode_ops.c"
#include "../src/data_block_ops.c"

#include "test_helpers.c"

#define DATABLOCK_LAYER_TEST "altfs_datablock_layer_test"

int main()
{
    printf("=============== TESTING DATA BLOCK & INODE OPERATIONS =============\n\n");
    // Create filesystem (assumes superblock layer tests pass)
    if(!altfs_makefs())
    {
        printf("Altfs makefs failed!");
        return -1;
    }

    /*
    * Check data block ops.
    */
    fprintf(stdout, "\n******************* TESTING DATA BLOCK OPERATIONS *********************\n");

    char* buffer = (char*)malloc(BLOCK_SIZE);
    char* str = "I really hope my block layer works!";
    memcpy(buffer, str, strlen(str));

    fprintf(stdout, "%s : Attemting to CRUD %ld data blocks.\n", DATABLOCK_LAYER_TEST, NUM_OF_ADDRESSES_PER_BLOCK);
    for(ssize_t i = 0; i < NUM_OF_ADDRESSES_PER_BLOCK; i++)
    {
        // Allocate new data block
        ssize_t block_num = allocate_data_block();
        if(block_num < 0)
        {
            fprintf(stderr, "%s : Failed to create data block at i = %ld.\n", DATABLOCK_LAYER_TEST, i);
            return -1;
        }
        // Write to the new data block
        if(!write_data_block(block_num, buffer))
        {
            fprintf(stderr, "%s : Failed to write data block at i = %ld.\n", DATABLOCK_LAYER_TEST, i);
            return -1;
        }
        // Read the data block
        buffer = read_data_block(block_num);
        if(buffer == NULL)
        {
            fprintf(stderr, "%s : Failed to read data block at i = %ld.\n", DATABLOCK_LAYER_TEST, i);
            return -1;
        }
        if(strcmp(buffer, str) != 0)
        {
            fprintf(stderr, "%s : Read data from block does not match control string at i = %ld.\n", DATABLOCK_LAYER_TEST, i);
            return -1;
        }
    }
    fprintf(stdout, "%s : Create, update, read verified.\n", DATABLOCK_LAYER_TEST);
    free(buffer);

    // Check freelist consistency
    char *sb_buf = (char*)malloc(BLOCK_SIZE);
    if (!altfs_read_block(0, sb_buf))
    {
        fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DATABLOCK_LAYER_TEST);
        return -1;
    }
    struct superblock* sb = (struct superblock*)sb_buf;
    if(sb->s_freelist_head != INODE_BLOCK_COUNT + 1 + NUM_OF_ADDRESSES_PER_BLOCK)
    {
        fprintf(stderr, "%s : Freelist head inconsistent after CRUDS.\n", DATABLOCK_LAYER_TEST);
        fprintf(stderr, "%s : Freelist head: %ld | Should be: %ld.\n", DATABLOCK_LAYER_TEST,
            sb->s_freelist_head, (INODE_BLOCK_COUNT + 1 + NUM_OF_ADDRESSES_PER_BLOCK));
        return -1;
    }
    fprintf(stdout, "%s : Freelist consistent!.\n", DATABLOCK_LAYER_TEST);

    fprintf(stdout, "%s : Freeing the first non-reserved data block.\n", DATABLOCK_LAYER_TEST);
    // Free the initial datablock
    if(!free_data_block(INODE_BLOCK_COUNT + 1))
    {
        fprintf(stderr, "%s : Error while free-ing data block.\n", DATABLOCK_LAYER_TEST);
        return -1;
    }
    fprintf(stdout, "%s : Free operation verified.\n", DATABLOCK_LAYER_TEST);
    // Check freelist consistency
    if (!altfs_read_block(0, sb_buf))
    {
        fprintf(stderr, "%s : Failed to read block 0 for superblock\n", DATABLOCK_LAYER_TEST);
        return -1;
    }
    sb = (struct superblock*)sb_buf;
    if(sb->s_freelist_head != INODE_BLOCK_COUNT + 1)
    {
        fprintf(stderr, "%s : Freelist head inconsistent after free_data_block.\n", DATABLOCK_LAYER_TEST);
        fprintf(stderr, "%s : Freelist head: %ld | Should be: %ld.\n", DATABLOCK_LAYER_TEST,
            sb->s_freelist_head, (INODE_BLOCK_COUNT + 1));
        return -1;
    }
    fprintf(stdout, "%s : Freelist consistent!.\n", DATABLOCK_LAYER_TEST);
    free(sb_buf);

    /*
    * Check inode ops.
    */
   fprintf(stdout, "\n******************* TESTING INODE OPERATIONS *********************\n");
    ssize_t inum = 0;

    fprintf(stdout, "%s : Attempting to allocate %ld inodes.\n", DATABLOCK_LAYER_TEST, sb->s_num_of_inodes_per_block);
    for(ssize_t i = 0; i < sb->s_num_of_inodes_per_block; i++)
    {
        inum = allocate_inode();
        if(!is_valid_inode_number(inum))
        {
            fprintf(stderr, "%s : Failed to create inode at i = %ld.\n", DATABLOCK_LAYER_TEST, i);
            return -1;
        }
    }
    fprintf(stdout, "%s : Allocate inode verified.\n", DATABLOCK_LAYER_TEST);

    fprintf(stdout, "%s : Building a new inode to write.\n", DATABLOCK_LAYER_TEST);
    ssize_t assigned_dblocks[4]; // hold what is being assigned to iblocks 3, 12, 15, 524
    struct inode* node = (struct inode*)malloc(sizeof(struct inode));
    for(ssize_t i = 0; i < NUM_OF_DIRECT_BLOCKS; i++)
    {
        node->i_direct_blocks[i] = allocate_data_block();
        fprintf(stdout, "%s : Allocate data block number %ld.\n", DATABLOCK_LAYER_TEST, node->i_direct_blocks[i]);
        if(i == 3)
            assigned_dblocks[0] = node->i_direct_blocks[3];
    }

    node->i_single_indirect = allocate_data_block();
    fprintf(stdout, "%s : Single indirect block number %ld.\n", DATABLOCK_LAYER_TEST, node->i_single_indirect);
    char single_i_buf[BLOCK_SIZE];
    memset(single_i_buf, 0, BLOCK_SIZE);
    size_t offset = 0;
    for(ssize_t i = 0; i < 5; i++)
    {
        ssize_t block_num = allocate_data_block();
        fprintf(stdout, "%s : Allocate data block number %ld.\n", DATABLOCK_LAYER_TEST, block_num);
        memcpy(single_i_buf + offset, &block_num, ADDRESS_SIZE);
        offset += ADDRESS_SIZE;
        if(i == 0)
            assigned_dblocks[1] = block_num;
        else if(i == 3)
            assigned_dblocks[2] = block_num;
    }
    if(!write_data_block(node->i_single_indirect, single_i_buf))
    {
        fprintf(stderr, "%s : Failed to write single indirect block.\n", DATABLOCK_LAYER_TEST);
        return -1;
    }

    node->i_double_indirect = allocate_data_block();
    fprintf(stdout, "%s : Double indirect block number %ld.\n", DATABLOCK_LAYER_TEST, node->i_double_indirect);
    char double_i_buf[BLOCK_SIZE];
    memset(double_i_buf, 0, BLOCK_SIZE);
    ssize_t i_one_block_num = allocate_data_block();
    fprintf(stdout, "%s : Double-1 indirect block number %ld.\n", DATABLOCK_LAYER_TEST, i_one_block_num);
    memcpy(double_i_buf, &i_one_block_num, ADDRESS_SIZE);
    if(!write_data_block(node->i_double_indirect, double_i_buf))
    {
        fprintf(stderr, "%s : Failed to write double indirect block.\n", DATABLOCK_LAYER_TEST);
        return -1;
    }
    memset(single_i_buf, 0, BLOCK_SIZE);
    ssize_t i_two_block_num = allocate_data_block();
    fprintf(stdout, "%s : Allocate data block number %ld.\n", DATABLOCK_LAYER_TEST, i_two_block_num);
    memcpy(single_i_buf, &i_two_block_num, ADDRESS_SIZE);
    if(!write_data_block(i_one_block_num, single_i_buf))
    {
        fprintf(stderr, "%s : Failed to write double indirect block.\n", DATABLOCK_LAYER_TEST);
        return -1;
    }
    assigned_dblocks[3] = i_two_block_num;
    node->i_allocated = true;
    node->i_blocks_num = 524;

    // Print the constructed inode
    print_inode(&node);

    // Write the inode to disc
    fprintf(stdout, "%s : Writing the inode to disc.\n", DATABLOCK_LAYER_TEST);
    if(!write_inode(inum, node))
    {
        fprintf(stderr, "%s : Failed to write inode %ld to disc.\n", DATABLOCK_LAYER_TEST, inum);
        return -1;
    }
    fprintf(stdout, "%s : Write inode verified.\n\n", DATABLOCK_LAYER_TEST);
    free(node);

    // Read the inode form disc
    fprintf(stdout, "%s : Reading the inode from disc.\n", DATABLOCK_LAYER_TEST);
    struct inode* read_inode = get_inode(inum);
    if(read_inode == NULL)
    {
        fprintf(stderr, "%s : Unable to read inode %ld from disc.\n", DATABLOCK_LAYER_TEST, inum);
        altfs_free_memory(read_inode);
        return -1;
    }
    print_inode(&read_inode);
    fprintf(stdout, "%s : Read inode verified.\n\n", DATABLOCK_LAYER_TEST);

    // Check if the iblock num to dblock num mapping works fine
    fprintf(stdout, "%s : Fetching physical block numbers for logical block numbers from inode.\n", DATABLOCK_LAYER_TEST);
    ssize_t iblock_nums[4] = {3, 12, 15, 524};
    for(int i = 0; i < 4; i++){
        ssize_t prev = 0;
        ssize_t pblock_num = get_disk_block_from_inode_block(read_inode, iblock_nums[i], &prev);
        if(pblock_num != assigned_dblocks[i])
        {
            fprintf(stderr, "%s : logical and physical mapping utility mismatch. Logical block %ld should be physical %ld but got %ld.\n", DATABLOCK_LAYER_TEST, iblock_nums[i], assigned_dblocks[i], pblock_num);
            altfs_free_memory(read_inode);
            return -1;
        }
    }
    fprintf(stdout, "%s :Logical and physical mapping utility verified.\n\n", DATABLOCK_LAYER_TEST);
    altfs_free_memory(read_inode);

    // Free the inode
    unsigned long long free_blocks_init = get_num_of_free_blocks();
    fprintf(stdout, "%s : Freeing the inode %ld.\n", DATABLOCK_LAYER_TEST, inum);
    if(!free_inode(inum))
    {
        fprintf(stderr, "%s : Unable to free inode %ld.\n", DATABLOCK_LAYER_TEST, inum);
        return -1;
    }
    unsigned long long free_blocks_final = get_num_of_free_blocks();
    ssize_t blocks_freed = free_blocks_final - free_blocks_init;
    printf("%s : Should have freed 21 blocks. Freed: %ld\n", DATABLOCK_LAYER_TEST, blocks_freed);

    struct inode* read_node = get_inode(inum);
    if( read_node->i_allocated != 0 ||
        read_node->i_blocks_num != 0 ||
        read_node->i_child_num != 0 ||
        read_node->i_file_size != 0 ||
        read_node->i_links_count != 0 ||
        read_node->i_mode != 0 )
    {
        fprintf(stderr, "%s : Inode %ld not freed. Details:\n", DATABLOCK_LAYER_TEST, inum);
        print_inode(&read_node);
        altfs_free_memory(read_node);
        return -1;
    }
    altfs_free_memory(read_node);

    // Free a smaller value inode
    inum = 4;
    fprintf(stdout, "%s : Freeing the inode %ld.\n", DATABLOCK_LAYER_TEST, inum);
    if(!free_inode(inum))
    {
        fprintf(stderr, "%s : Unable to free inode %ld.\n", DATABLOCK_LAYER_TEST, inum);
        return -1;
    }

    fprintf(stdout, "%s : Free inode verified.\n\n", DATABLOCK_LAYER_TEST);

    printf("=============== ALL TESTS RUN ================\n\n");
    return 0;
}
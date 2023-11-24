#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"
#include "../src/inode_ops.c"
#include "../src/data_block_ops.c"

#define DATABLOCK_LAYER_TEST "altfs_datablock_layer_test"

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
    printf("\n******************** INODE ********************\n");
}

int main()
{
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
    struct inode* node = (struct inode*)malloc(sizeof(struct inode));
    for(ssize_t i = 0; i < NUM_OF_DIRECT_BLOCKS; i++)
    {
        node->i_direct_blocks[i] = allocate_data_block();
        fprintf(stdout, "%s : Allocate data block number %ld.\n", DATABLOCK_LAYER_TEST, node->i_direct_blocks[i]);
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

    // Print the constructed inode
    print_inode(&node);

    // Write the inode to disc
    fprintf(stdout, "%s : Writing the inode to disc.\n", DATABLOCK_LAYER_TEST);
    if(!write_inode(inum, node))
    {
        fprintf(stderr, "%s : Failed to write inode %ld to disc.\n", DATABLOCK_LAYER_TEST, inum);
        return -1;
    }
    fprintf(stdout, "%s : Write inode verified.\n", DATABLOCK_LAYER_TEST);
    free(node);

    // Read the inode form disc
    fprintf(stdout, "%s : Reading the inode from disc.\n", DATABLOCK_LAYER_TEST);
    struct inode* read_inode = get_inode(inum);
    if(read_inode == NULL)
    {
        fprintf(stderr, "%s : Unable to read inode %ld from disc.\n", DATABLOCK_LAYER_TEST, inum);
        return -1;
    }
    print_inode(&read_inode);
    fprintf(stdout, "%s : Read inode verified.\n", DATABLOCK_LAYER_TEST);

    // Free the inode
    fprintf(stdout, "%s : Freeing the inode.\n", DATABLOCK_LAYER_TEST);
    if(!free_inode(inum))
    {
        fprintf(stderr, "%s : Unable to free inode %ld.\n", DATABLOCK_LAYER_TEST, inum);
        return -1;
    }

    fprintf(stdout, "%s : Free inode verified.\n", DATABLOCK_LAYER_TEST);

    return 0;
}
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"
#include "../src/inode_ops.c"
#include "../src/data_block_ops.c"
#include "../src/inode_data_block_ops.c"

#define INODE_DATA_BLOCK_OPS "altfs_inode_data_block_ops_test"

// Test adding new datablocks to inode
int test_add_data_block_to_inode()
{   
    fprintf(stdout, "\n=============== START: TESTING INODE DATA BLOCK OPERATIONS =============\n");

    // allocate inode and verify new data blocks can be added
    ssize_t inum = 0;

    inum = allocate_inode();
    if(!is_valid_inode_number(inum))
    {
        fprintf(stderr, "%s : Failed to create inode.\n", INODE_DATA_BLOCK_OPS, i);
        return -1;
    }
    fprintf(stdout, "%s : Allocated inode with number %ld\n", INODE_DATA_BLOCK_OPS, inum);

    struct inode* node = get_inode(inum);
    ssize_t data_block_num = allocate_data_block();
    if (!data_block_num)
    {
        fprintf(stderr, "%s : Failed to allocate data block for inode %ld\n", INODE_DATA_BLOCK_OPS, inum);
        return -1;
    }
    fprintf("%s : Allocated new data block %ld\n", INODE_DATA_BLOCK_OPS, data_block_num);

    if (!add_datablock_to_inode(node, data_block_num))
    {
        fprintf(stderr, "%s : Failed to associate data block %ld to inode %ld\n",INODE_DATA_BLOCK_OPS, data_block_num, inum);
        return -1;
    }
    fprintf("%s : Associated data block %ld with inum %ld\n", INODE_DATA_BLOCK_OPS, data_block_num, inum);
    fprintf(stdout, "\n=============== END: TESTING INODE DATA BLOCK OPERATIONS =============\n");
    return 0;
}

int main()
{
    // Create filesystem (assumes superblock layer tests pass)
    if(!altfs_makefs())
    {
        printf("Altfs makefs failed!");
        return -1;
    }

    if (test_add_data_block_to_inode() == -1)
        fprintf(stderr, "%s : Test1 testing inode data block ops failed\n", INODE_DATA_BLOCK_OPS);
    
    return 0;
}
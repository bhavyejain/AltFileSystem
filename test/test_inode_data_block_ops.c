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

void print_inode_data_blocks(struct inode *node, ssize_t inum)
{
    // Verify that all direct blocks are associated correctly with inode
    for(int i=0; i < NUM_OF_DIRECT_BLOCKS; i++)
        fprintf(stdout, "%s : direct block %ld for inum %ld = %ld\n", INODE_DATA_BLOCK_OPS, i, inum, node->i_direct_blocks[i]);

    fprintf(stdout, "%s : single indirect block %ld for inum %ld\n", INODE_DATA_BLOCK_OPS, node->i_single_indirect, inum);
    
    if (node->i_single_indirect == 0)
        return;

    fprintf(stdout, "\n************ BLOCK ADDRESSES IN SINGLE INDIRECT BLOCK: %ld *************\n",node->i_single_indirect);
    char *buff = (char*)malloc(BLOCK_SIZE);
    if (!altfs_read_block(node->i_single_indirect, buff))
    {
        fprintf(stderr, "%s : Error reading contents of block number: %ld\n",INODE_DATA_BLOCK_OPS, node->i_single_indirect);
        return -1;
    }
    ssize_t *buff_numptr = (ssize_t *)buff;
    altfs_free_memory(buff);

    for (ssize_t i = 0; i < NUM_OF_ADDRESSES_PER_BLOCK / 8; i++)
    {
        for (ssize_t j = 0; j < 8; j++)
        {
            fprintf(stdout, "%ld ", buff_numptr[i * 8 + j]);
        }
        fprintf(stdout,"\n");
    }

    fprintf(stdout, "\n************ BLOCK ADDRESSES IN DOUBLE INDIRECT BLOCK: %ld *************\n",node->i_double_indirect);
    
    ssize_t *double_indirect_block_arr = (ssize_t*) read_data_block(node->i_double_indirect);
    if (!double_indirect_block_arr)
    {
        fprintf(stderr, "%s : Error reading contents of block number: %ld\n",INODE_DATA_BLOCK_OPS, node->i_double_indirect);
        return -1;
    }

    for(ssize_t i = 0; i < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR; i++)
    {
        ssize_t data_block_num = double_indirect_block_arr[i];
        if(data_block_num <= 0)
        {
            fprintf(stdout, "Double indirect block num <= 0.\n");
            return -1;
        }
        fprintf(stdout, "Single indirect block num %zd\n", data_block_num);

        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);
        for (ssize_t j = 0; j < NUM_OF_ADDRESSES_PER_BLOCK / 8; j++)
        {
            for (ssize_t k = 0; k < 8; k++)
            {
                fprintf(stdout, "%ld ", single_indirect_block_arr[j * 8 + k]);
            }
            fprintf(stdout,"\n");
        }
    }
    /*char *buff2 = (char*)malloc(BLOCK_SIZE);

    for(ssize_t i = 0; i < NUM_OF_ADDRESSES_PER_BLOCK; i++)
    {
        fprintf(stdout, "Trying to read indirect block: %zd\n", buff_numptr[i]);
        if (!altfs_read_block(buff_numptr[i], buff2))
        {
            fprintf(stderr, "%s : Error reading contents of block number: %ld\n",INODE_DATA_BLOCK_OPS, buff_numptr[i]);
            return -1;
        }
        ssize_t *buff_numptr2 = (ssize_t *)buff2;
        fprintf(stdout, "Single indirect block addr: %zd\n", buff_numptr2);
        for(ssize_t j = 0; j < NUM_OF_ADDRESSES_PER_BLOCK / 8; j++)
        {
            for(ssize_t k = 0; k < 8; k++)
            {
                fprintf(stdout, "%zd", buff_numptr2[j * 8 + k]);
            }
            fprintf(stdout, "\n");
        }
    }*/
}

int allocate_datablocks_util(struct inode *node, int num_of_blocks, ssize_t inum)
{
    for(int i = 0; i < num_of_blocks; i++)
    {
        ssize_t data_block_num = allocate_data_block();
        if (!data_block_num)
        {
            fprintf(stderr, "%s : Failed to allocate data block for inode %ld\n", INODE_DATA_BLOCK_OPS, inum);
            return -1;
        }
        fprintf(stdout, "%s : Allocated new data block %ld\n", INODE_DATA_BLOCK_OPS, data_block_num);

        if (!add_datablock_to_inode(node, data_block_num))
        {
            fprintf(stderr, "%s : Failed to associate data block %ld to inode %ld\n",INODE_DATA_BLOCK_OPS, data_block_num, inum);
            return -1;
        }
        fprintf(stdout, "%s : Associated data block %ld with inum %ld\n", INODE_DATA_BLOCK_OPS, data_block_num, inum);

    }
    return 0;
}

// Test adding new datablocks to inode
int test_add_data_block_to_inode()
{   
    fprintf(stdout, "\n=============== START: TESTING INODE DATA BLOCK OPERATIONS =============\n");

    // allocate inode and verify new data blocks can be added
    ssize_t inum = 0;

    inum = allocate_inode();
    if(!is_valid_inode_number(inum))
    {
        fprintf(stderr, "%s : Failed to create inode.\n", INODE_DATA_BLOCK_OPS);
        return -1;
    }
    fprintf(stdout, "%s : Allocated inode with number %ld\n", INODE_DATA_BLOCK_OPS, inum);

    struct inode* node = get_inode(inum);
    int num_of_blocks_to_allocate = 20 + 512;
    // Allocate 20+512 data blocks to inode
    for(int i = 0; i < num_of_blocks_to_allocate; i++)
    {
        ssize_t data_block_num = allocate_data_block();
        if (!data_block_num)
        {
            fprintf(stderr, "%s : Failed to allocate data block for inode %ld\n", INODE_DATA_BLOCK_OPS, inum);
            return -1;
        }
        fprintf(stdout, "%s : Allocated new data block %ld\n", INODE_DATA_BLOCK_OPS, data_block_num);

        if (!add_datablock_to_inode(node, data_block_num))
        {
            fprintf(stderr, "%s : Failed to associate data block %ld to inode %ld\n",INODE_DATA_BLOCK_OPS, data_block_num, inum);
            return -1;
        }
        fprintf(stdout, "%s : Associated data block %ld with inum %ld\n", INODE_DATA_BLOCK_OPS, data_block_num, inum);

    }
    /*if (!allocate_datablocks_util(node, num_of_blocks_to_allocate, inum))
    {
        fprintf(stderr, "Failed to allocate %d blocks to inode %zd\n", num_of_blocks_to_allocate, inum);
        return -1;
    }*/

    fprintf(stdout, "%s: Allocated %ld data blocks to inode\n", INODE_DATA_BLOCK_OPS, num_of_blocks_to_allocate);
    print_inode_data_blocks(node, inum);

    // Verify removing data blocks - it removes data blocks starting from the given block number until the end
    // Test removing block 3, 12, 15
    ssize_t blocks_to_remove[] = {3,12,15};
    ssize_t n = sizeof(blocks_to_remove)/sizeof(blocks_to_remove[0]);
    for(int i = n-1; i >= 0; i--)
    {
        if (!remove_datablocks_from_inode(node, blocks_to_remove[i]))
        {
            fprintf(stderr, "%s : Failed to remove data block %ld onwards from inum %ld\n", INODE_DATA_BLOCK_OPS, blocks_to_remove[i], inum);
            return -1;
        }
        fprintf(stdout, "%s : Removed data blocks from %ld onwards from inode %ld\n", INODE_DATA_BLOCK_OPS, blocks_to_remove[i], inum);
        print_inode_data_blocks(node, inum);

        // reallocate removed data blocks
        /*for(int j = 0; j < num_of_blocks_to_allocate - blocks_to_remove[i]; j++)
        {
            ssize_t data_block_num = allocate_data_block();
            if (!data_block_num)
            {
                fprintf(stderr, "%s : Failed to allocate data block for inode %ld\n", INODE_DATA_BLOCK_OPS, inum);
                return -1;
            }
            fprintf(stdout, "%s : Allocated new data block %ld\n", INODE_DATA_BLOCK_OPS, data_block_num);

            if (!add_datablock_to_inode(node, data_block_num))
            {
                fprintf(stderr, "%s : Failed to associate data block %ld to inode %ld\n",INODE_DATA_BLOCK_OPS, data_block_num, inum);
                return -1;
            }
            fprintf(stdout, "%s : Associated data block %ld with inum %ld\n", INODE_DATA_BLOCK_OPS, data_block_num, inum);
        }*/
    }
    
    fprintf(stdout, "\n=============== END: TESTING INODE DATA BLOCK OPERATIONS =============\n");
    return 0;
}

int main()
{
    printf("=============== TESTING INODE DATA BLOCK OPERATIONS =============\n\n");
    // Create filesystem (assumes superblock layer tests pass)
    if(!altfs_makefs())
    {
        printf("Altfs makefs failed!");
        return -1;
    }

    if (test_add_data_block_to_inode() == -1)
        fprintf(stderr, "%s : Testing add data block to inode failed\n", INODE_DATA_BLOCK_OPS);

    return 0;
}
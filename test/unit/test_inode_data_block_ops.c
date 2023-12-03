#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../../src/disk_layer.c"
#include "../../src/superblock_layer.c"
#include "../../src/inode_ops.c"
#include "../../src/data_block_ops.c"
#include "../../src/inode_data_block_ops.c"

#define INODE_DATA_BLOCK_OPS "altfs_inode_data_block_ops_test"

void print_inode_data_blocks(struct inode *node, ssize_t inum)
{
    fprintf(stdout, "\n************ PRINT INODE DATA BLOCKS *************\n");
    // Verify that all direct blocks are associated correctly with inode
    for(int i=0; i < NUM_OF_DIRECT_BLOCKS; i++)
        fprintf(stdout, "%s : direct block %d for inum %ld = %ld\n", INODE_DATA_BLOCK_OPS, i, inum, node->i_direct_blocks[i]);

    fprintf(stdout, "%s : single indirect block %ld for inum %ld\n", INODE_DATA_BLOCK_OPS, node->i_single_indirect, inum);
    
    if (node->i_single_indirect == 0)
        return;

    fprintf(stdout, "\n************ BLOCK ADDRESSES IN SINGLE INDIRECT BLOCK: %ld *************\n",node->i_single_indirect);
    char *buff = (char*)malloc(BLOCK_SIZE);
    if (!altfs_read_block(node->i_single_indirect, buff))
    {
        fprintf(stderr, "%s : Error reading contents of block number: %ld\n",INODE_DATA_BLOCK_OPS, node->i_single_indirect);
        return;
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

    if (node->i_double_indirect == 0)
        return;

    fprintf(stdout, "\n************ BLOCK ADDRESSES IN DOUBLE INDIRECT BLOCK: %ld *************\n",node->i_double_indirect);

    ssize_t *double_indirect_block_arr = (ssize_t*) read_data_block(node->i_double_indirect);
    if (!double_indirect_block_arr)
    {
        fprintf(stderr, "%s : Error reading contents of block number: %ld\n",INODE_DATA_BLOCK_OPS, node->i_double_indirect);
        return;
    }

    for(ssize_t i = 0; i < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR; i++)
    {
        ssize_t data_block_num = double_indirect_block_arr[i];
        if (data_block_num == 0)
            break;
        
        if(data_block_num < 0)
        {
            fprintf(stdout, "Double indirect block num <= 0.\n");
            return;
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
    fprintf(stdout, "\n************ PRINT INODE DATA BLOCKS *************\n");
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

    #ifdef DISK_MEMORY
        ssize_t num_of_blocks_to_allocate[] = {8, 12, 100, 524, 530, 1044, 2080, 270000, 550000};
    #else
        ssize_t num_of_blocks_to_allocate[] = {8, 12, 100, 524, 530, 1044, 2080};
    #endif 
    
    for(int k = 0; k < sizeof(num_of_blocks_to_allocate)/sizeof(num_of_blocks_to_allocate[0]); k++)
    {
        for(int i = 0; i < num_of_blocks_to_allocate[k]; i++)
        {
            ssize_t data_block_num = allocate_data_block();
            if (!data_block_num)
            {
                fprintf(stderr, "%s : Failed to allocate data block for inode %ld\n", INODE_DATA_BLOCK_OPS, inum);
                return -1;
            }

            if (!add_datablock_to_inode(node, data_block_num))
            {
                fprintf(stderr, "%s : Failed to associate data block %ld to inode %ld\n",INODE_DATA_BLOCK_OPS, data_block_num, inum);
                return -1;
            }

        }

        fprintf(stdout, "\n ================================== ALLOCATED %zd BLOCKS ==================================\n", num_of_blocks_to_allocate[k]);
        //print_inode_data_blocks(node, inum); TODO: Uncomment later

        // Verify removing data blocks - it removes data blocks starting from the given block number until the end
        // NOTE - logical block numbers start from 0
        
        #ifdef DISK_MEMORY
            ssize_t logical_blocks_to_remove_from[] = {3, 7, 8, 11, 12, 14, 16, 60, 523, 524, 526, 530, 1040, 1600, 2070, 265000, 300000, 540000}; 
        #else
            ssize_t logical_blocks_to_remove_from[] = {3, 7, 8, 11, 12, 14, 16, 60, 523, 524, 526, 530, 1040, 1600, 2070}; 
        #endif 
        
        ssize_t n = sizeof(logical_blocks_to_remove_from)/sizeof(logical_blocks_to_remove_from[0]);
        for(int i = 0; i < n; i++)
        {
            fprintf(stdout, "\n\n ********************** Logical blocks to remove from: %ld Blocks allocated: %ld **********************\n\n", logical_blocks_to_remove_from[i], num_of_blocks_to_allocate[k]);
            if (logical_blocks_to_remove_from[i] >= num_of_blocks_to_allocate[k])
            {
                if (!remove_datablocks_from_inode(node, 0))
                {
                    fprintf(stderr, "%s : Failed to remove data block %ld onwards from inum %ld\n", INODE_DATA_BLOCK_OPS, logical_blocks_to_remove_from[i], inum);
                    return -1;
                }
                break;
            }

            if (!remove_datablocks_from_inode(node, logical_blocks_to_remove_from[i]))
            {
                fprintf(stderr, "%s : Failed to remove data block %ld onwards from inum %ld\n", INODE_DATA_BLOCK_OPS, logical_blocks_to_remove_from[i], inum);
                return -1;
            }
            fprintf(stdout, "%s : Removed data blocks from %ld onwards from inode %ld\n", INODE_DATA_BLOCK_OPS, logical_blocks_to_remove_from[i], inum);
            // print_inode_data_blocks(node, inum); TODO: Uncomment later

            // reallocate removed data blocks
            fprintf(stdout, "\n%s: Reallocating removed blocks\n", INODE_DATA_BLOCK_OPS);
            for(int j = 0; j < num_of_blocks_to_allocate[k] - logical_blocks_to_remove_from[i]; j++)
            {
                ssize_t data_block_num = allocate_data_block();
                if (!data_block_num)
                {
                    fprintf(stderr, "%s : Failed to allocate data block for inode %ld\n", INODE_DATA_BLOCK_OPS, inum);
                    return -1;
                }

                if (!add_datablock_to_inode(node, data_block_num))
                {
                    fprintf(stderr, "%s : Failed to associate data block %ld to inode %ld\n",INODE_DATA_BLOCK_OPS, data_block_num, inum);
                    return -1;
                }
            }
            fprintf(stdout, "\n************ REALLOCATED BLOCKS *************\n");
            // print_inode_data_blocks(node, inum); TODO: Uncomment later
        } 
        fprintf(stdout, "\n ================================== REMOVED BLOCKS FROM %zd ALLOCATED BLOCKS ==================================\n", num_of_blocks_to_allocate[k]);
    }
    
    fprintf(stdout, "\n=============== END: TESTING INODE DATA BLOCK OPERATIONS =============\n");
    return 0;
}

int main()
{
    printf("=============== TESTING INODE DATA BLOCK OPERATIONS =============\n\n");
    // Create filesystem (assumes superblock layer tests pass)
    #ifndef DISK_MEMORY
    if(!altfs_makefs())
    {
        printf("Altfs makefs failed!");
        return -1;
    }
    #endif

    if (test_add_data_block_to_inode() == -1)
        fprintf(stderr, "%s : Testing add data block to inode failed\n", INODE_DATA_BLOCK_OPS);

    teardown();
    return 0;
}
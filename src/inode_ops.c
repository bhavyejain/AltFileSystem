#include "../header/data_block_ops.h"
#include "../header/disk_layer.h"
#include "../header/inode_ops.h"

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

bool is_valid_inode_number(ssize_t inum)
{
    return inum > 0 && inum < altfs_superblock->s_inodes_count;
}

/*
Given the inode number, get which block contains it and what is the offset in the block.
*/
void inum_to_block_pos(ssize_t inum, ssize_t* block, ssize_t* offset)
{
    ssize_t tmp = altfs_superblock->s_num_of_inodes_per_block;
    *block = 1 + (inum / tmp);
    *offset = inum % tmp;
}

ssize_t allocate_inode()
{
    fuse_log(FUSE_LOG_DEBUG, "%s Attempting to allocate a new inode.\n", ALLOCATE_INODE);
    // Get the next free inode number
    ssize_t inum_to_allocate = altfs_superblock->s_first_ino;
    if(inum_to_allocate == altfs_superblock->s_inodes_count)
    {
        fuse_log(FUSE_LOG_ERR, "%s All inodes are allocated.", ALLOCATE_INODE);
        return -1;
    }

    if(!is_valid_inode_number(inum_to_allocate))
    {
        fuse_log(FUSE_LOG_ERR, "%s Invalid inode number provided for allocation:%ld\n", ALLOCATE_INODE, altfs_superblock->s_first_ino);
        return -1;
    }

    // Get block number and offset for the inode number, and read the block.
    ssize_t block_num, offset;
    inum_to_block_pos(inum_to_allocate, &block_num, &offset);
    char buffer[BLOCK_SIZE];
    if(!altfs_read_block(block_num, buffer))
    {
        fuse_log(FUSE_LOG_ERR, "%s Error reading data block number %ld\n", ALLOCATE_INODE, block_num);
        return -1;
    }

    struct inode* nodes_in_block = NULL;
    struct inode* node = NULL;

    nodes_in_block = (struct inode*)buffer;
    node = nodes_in_block + offset;
    // Some error in updating next free inode previously.
    if(node->i_allocated)
    {
        fuse_log(FUSE_LOG_ERR, "%s Inode %ld is already allocated.\n", ALLOCATE_INODE, inum_to_allocate);
        return -1;
    }
    // Mark the inode as allocated.
    node->i_allocated = true;
    fuse_log(FUSE_LOG_DEBUG, "%s Allocated inum: %ld (block: %ld; offset: %ld)\n",
        ALLOCATE_INODE, inum_to_allocate, block_num, offset);

    // Search for the next smallest inode number that is unallocated.
    ssize_t visited = 0;
    offset++;
    ssize_t inodes_left = altfs_superblock->s_inodes_count - inum_to_allocate + 1;
    while(visited != inodes_left)
    {
        while(offset < altfs_superblock->s_num_of_inodes_per_block)
        {
            node = nodes_in_block + offset;
            visited++;
            if(!node->i_allocated)
            {
                altfs_superblock->s_first_ino = inum_to_allocate + visited;
                fuse_log(FUSE_LOG_DEBUG, "%s Marking inode %ld as next free inode.\n",
                    ALLOCATE_INODE, altfs_superblock->s_first_ino);
                return inum_to_allocate;
            }
            offset++;
        }

        offset = 0;
        block_num++;
        if(block_num > INODE_BLOCK_COUNT)
        {
            break;
        }

        if(!altfs_read_block(block_num, buffer))
        {
            fuse_log(FUSE_LOG_ERR, "%s Error reading data block number %ld\n", ALLOCATE_INODE, block_num);
            break;
        }
        nodes_in_block = (struct inode*)buffer;
    }

    fuse_log(FUSE_LOG_DEBUG, "%s All inodes allocated.\n", ALLOCATE_INODE);
    altfs_superblock->s_first_ino = altfs_superblock->s_inodes_count;
    return inum_to_allocate;
}

struct inode* get_inode(ssize_t inum)
{
    fuse_log(FUSE_LOG_DEBUG, "%s Attempting to get inode %ld\n", GET_INODE, inum);

    if(!is_valid_inode_number(inum)){
        fuse_log(FUSE_LOG_ERR, "%s Invalid inode number provided for allocation:%ld\n", GET_INODE, altfs_superblock->s_first_ino);
        return NULL;
    }

    ssize_t block_num, offset;
    inum_to_block_pos(inum, &block_num, &offset);

    char buffer[BLOCK_SIZE];
    if(!altfs_read_block(block_num, buffer)){
        fuse_log(FUSE_LOG_ERR, "%s Error reading data block number %ld\n", GET_INODE, block_num);
        return false;
    }

    struct inode* node = (struct inode*)buffer;
    node = node + offset;
    struct inode* requested_inode = (struct inode*)malloc(sizeof(struct inode));

    memcpy(requested_inode, node, sizeof(struct inode));
    return requested_inode;
}

bool write_inode(ssize_t inum, struct inode* node)
{
    fuse_log(FUSE_LOG_DEBUG, "%s Attempting to write to inode %ld\n", WRITE_INODE, inum);

    if(!is_valid_inode_number(inum)){
        fuse_log(FUSE_LOG_ERR, "%s Invalid inode number provided for write:%ld\n", WRITE_INODE, inum);
        return false;
    }

    ssize_t block_num, offset;
    inum_to_block_pos(inum, &block_num, &offset);

    char buffer[BLOCK_SIZE];
    if(!altfs_read_block(block_num, buffer)){
        fuse_log(FUSE_LOG_ERR, "%s Error reading data block number %ld\n", WRITE_INODE, block_num);
        return false;
    }

    struct inode* node_on_disc = (struct inode*)buffer;
    node_on_disc = node_on_disc + offset;

    memcpy(node_on_disc, node, sizeof(struct inode));
    if(!altfs_write_block(block_num, buffer)){
        fuse_log(FUSE_LOG_ERR, "%s Error writing data to block number %ld\n", WRITE_INODE, block_num);
        return false;
    }

    fuse_log(FUSE_LOG_DEBUG, "%s Written inode %ld in block %ld at offset %ld\n", WRITE_INODE,
        inum, block_num, offset);
    return true;
}

/*
Helper function to free data blocks at n levels of indirection. (1 <= n <= 3)
*/
bool free_indirect_blocks(ssize_t i_block_num, ssize_t indirection)
{
    // Read the data block to get the indirect data block numbers
    char buffer[BLOCK_SIZE];
    if(!altfs_read_block(i_block_num, buffer)){
        fuse_log(FUSE_LOG_ERR, "%s Error reading data block number %ld\n", FREE_INODE, i_block_num);
        return false;
    }
    ssize_t* data_blocks = (ssize_t*)buffer;

    // Iterate over every address (number)
    for(ssize_t i = 0; i < NUM_OF_ADDRESSES_PER_BLOCK; i++)
    {
        ssize_t block_num = data_blocks[i];
        if(block_num == 0)
        {
            break;
        }
        if(indirection == 1)
        {
            // fuse_log(FUSE_LOG_DEBUG, "%s Freeing indirect data block: %ld.\n", FREE_INODE, block_num);
            // Free the data block
            if(!free_data_block(block_num))
            {
                fuse_log(FUSE_LOG_ERR, "%s Error freeing data block: %ld\n", FREE_INODE, block_num);
                return false;
            }
        } else
        {
            // fuse_log(FUSE_LOG_DEBUG, "%s Freeing %ld-1 indirect data blocks starting at: %ld.\n", FREE_INODE, indirection, block_num);
            // Recursively call the function with a lower indirection
            if(!free_indirect_blocks(block_num, indirection-1))
            {
                fuse_log(FUSE_LOG_ERR, "%s Error freeing indirect data blocks at indirection %ld.\n", FREE_INODE, indirection);
                return false;
            }
        }
    }

    // fuse_log(FUSE_LOG_DEBUG, "%s Freeing the %ld indirect data block %ld itself.\n", FREE_INODE, indirection, i_block_num);
    // Free the block containing the indirect addresses itself
    if(!free_data_block(i_block_num))
    {
        fuse_log(FUSE_LOG_ERR, "%s Error freeing data block %ld\n", FREE_INODE, i_block_num);
        return false;
    }

    return true;
}

/*
Helper function to free all data blocks associated with an inode.
*/
bool free_data_blocks_in_inode(struct inode* node)
{
    // Free the direct blocks.
    for(ssize_t i = 0; i < NUM_OF_DIRECT_BLOCKS; i++)
    {
        ssize_t block_num = node->i_direct_blocks[i];
        if(block_num != 0)
        {
            if(!free_data_block(block_num))
            {
                fuse_log(FUSE_LOG_ERR, "%s Error freeing direct data block %ld\n", FREE_INODE, block_num);
                return false;
            }
        } else
        {
            break;
        }
    }

    // Free single indirect data blocks.
    if(node->i_single_indirect != 0)
    {
        if(!free_indirect_blocks(node->i_single_indirect, 1))
        {
            fuse_log(FUSE_LOG_ERR, "%s Error freeing single indirect data blocks.\n", FREE_INODE);
            return false;
        }
    }

    // Free double indirect data blocks.
    if(node->i_double_indirect != 0)
    {
        // fuse_log(FUSE_LOG_DEBUG, "%s Freeing double indirect data blocks starting at: %ld.\n", FREE_INODE, node->i_double_indirect);
        if(!free_indirect_blocks(node->i_double_indirect, 2))
        {
            fuse_log(FUSE_LOG_ERR, "%s Error freeing double indirect data blocks.\n", FREE_INODE);
            return false;
        }
    }

    // Free triple indirect data blocks.
    if(node->i_triple_indirect != 0)
    {
        if(!free_indirect_blocks(node->i_triple_indirect, 3))
        {
            fuse_log(FUSE_LOG_ERR, "%s Error freeing double indirect data blocks.\n", FREE_INODE);
            return false;
        }
    }

    return true;
}

bool free_inode(ssize_t inum)
{
    fuse_log(FUSE_LOG_DEBUG, "%s Attempting to free inode %ld\n", FREE_INODE, inum);

    if(!is_valid_inode_number(inum)){
        fuse_log(FUSE_LOG_ERR, "%s Invalid inode number provided for free-ing:%ld\n", FREE_INODE, inum);
        return false;
    }

    ssize_t block_num, offset;
    inum_to_block_pos(inum, &block_num, &offset);

    char buffer[BLOCK_SIZE];
    if(!altfs_read_block(block_num, buffer)){
        fuse_log(FUSE_LOG_ERR, "%s Error reading data block number %ld\n", FREE_INODE, block_num);
        return false;
    }

    struct inode* node = (struct inode*)buffer;
    node = node + offset;

    if(!free_data_blocks_in_inode(node))
    {
        fuse_log(FUSE_LOG_ERR, "%s Error freeing data blocks for inode %ld.\n", FREE_INODE, inum);
        return false;
    }

    node->i_mode = 0;
    node->i_uid = 0;
    node->i_gid = 0;
    node->i_status_change_time = 0;
    node->i_atime = 0;
    node->i_ctime = 0;
    node->i_mtime = 0;
    node->i_links_count = 0;
    node->i_file_size = 0;
    node->i_blocks_num = 0;
    node->i_allocated = false;
    for(ssize_t i = 0; i < NUM_OF_DIRECT_BLOCKS; i++)
        node->i_direct_blocks[i] = 0;
    node->i_single_indirect = 0;
    node->i_double_indirect = 0;
    node->i_triple_indirect = 0;
    node->i_child_num = 0;

    if(!altfs_write_block(block_num, buffer)){
        fuse_log(FUSE_LOG_ERR, "%s Error writing data to block number %ld\n", FREE_INODE, block_num);
        return false;
    }

    altfs_superblock->s_first_ino = min(inum, altfs_superblock->s_first_ino);
    fuse_log(FUSE_LOG_DEBUG, "%s Inode marked as next free: %ld\n", FREE_INODE, altfs_superblock->s_first_ino);
    return true;
}

ssize_t get_disk_block_from_inode_block(const struct inode* const node, ssize_t logical_block_num, ssize_t* prev_indirect_block)
{
    ssize_t data_block_num = -1;

    if(logical_block_num > node->i_blocks_num)
    {
        fuse_log(FUSE_LOG_ERR, "%s : File block number %ld is greater than data block count of file inode with %ld data blocks\n", GET_DBLOCK_FROM_IBLOCK, logical_block_num, node->i_blocks_num);
        return data_block_num;
    }
    
    // If file block is within direct block count, return data block number directly
    if(logical_block_num < NUM_OF_DIRECT_BLOCKS)
    {
        data_block_num = node->i_direct_blocks[logical_block_num];
        // UNCOMMENT
        // fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from direct block\n", GET_DBLOCK_FROM_IBLOCK, data_block_num);
        return data_block_num;
    }

    // Adjust logical block number for single indirect
    logical_block_num -= NUM_OF_DIRECT_BLOCKS;

    // If file block num < 512 => single indirect block
    if(logical_block_num < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)
    {
        if(node->i_single_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR,"%s : Single indirect block is set to 0 for inode.\n", GET_DBLOCK_FROM_IBLOCK);
            return data_block_num;
        }

        // Read single indirect block and extract data block num from file block num
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(node->i_single_indirect);
        data_block_num = single_indirect_block_arr[logical_block_num];
        altfs_free_memory(single_indirect_block_arr);

        fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from single indirect block\n", GET_DBLOCK_FROM_IBLOCK, data_block_num);
        return data_block_num;
    }

    // Adjust logical block number for double indirect
    logical_block_num -= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;
    
    // If file block num < 512*512 => double indirect block
    if(logical_block_num < NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR)
    {
        if(node->i_double_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR,"%s : Double indirect block is set to 0 for inode.\n", GET_DBLOCK_FROM_IBLOCK);
            return data_block_num;
        }
        ssize_t double_i_idx = logical_block_num / NUM_OF_ADDRESSES_PER_BLOCK;
        ssize_t inner_idx = logical_block_num % NUM_OF_ADDRESSES_PER_BLOCK;

        if(inner_idx != 0 && *prev_indirect_block != 0)
        {
            ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(*prev_indirect_block);
            data_block_num = single_indirect_block_arr[inner_idx];
            altfs_free_memory(single_indirect_block_arr);

            fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from double indirect block\n", GET_DBLOCK_FROM_IBLOCK, data_block_num);
            return data_block_num;
        }

        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(node->i_double_indirect);
        data_block_num = double_indirect_block_arr[double_i_idx];
        altfs_free_memory(double_indirect_block_arr);

        if(data_block_num <= 0){
            fuse_log(FUSE_LOG_ERR, "%s : Double indirect block num <= 0.\n", GET_DBLOCK_FROM_IBLOCK);
            return -1;
        }
        *prev_indirect_block = data_block_num;

        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);
        data_block_num = single_indirect_block_arr[inner_idx];
        altfs_free_memory(single_indirect_block_arr);

        fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from double indirect block\n", GET_DBLOCK_FROM_IBLOCK, data_block_num);
        return data_block_num;
    }

    // Adjust logical block number for double indirect
    logical_block_num -= NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;

    // If file block num < 512*512*512 => triple indirect block
    if(node->i_triple_indirect == 0)
    {
        fuse_log(FUSE_LOG_ERR,"%s : Triple indirect block is set to 0 for inode. Exiting\n", GET_DBLOCK_FROM_IBLOCK);
        return data_block_num;
    }

    ssize_t triple_i_idx = logical_block_num / NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;
    ssize_t double_i_idx = (logical_block_num / NUM_OF_ADDRESSES_PER_BLOCK) % NUM_OF_ADDRESSES_PER_BLOCK;
    ssize_t inner_idx = logical_block_num % NUM_OF_ADDRESSES_PER_BLOCK;

    if(inner_idx != 0 && *prev_indirect_block != 0)
    {
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(*prev_indirect_block);
        data_block_num = single_indirect_block_arr[inner_idx];
        altfs_free_memory(single_indirect_block_arr);

        fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from double indirect block\n", GET_DBLOCK_FROM_IBLOCK, data_block_num);
        return data_block_num;
    }

    ssize_t* triple_indirect_block_arr = (ssize_t*) read_data_block(node->i_triple_indirect);
    data_block_num = triple_indirect_block_arr[triple_i_idx];
    altfs_free_memory(triple_indirect_block_arr);

    if(data_block_num <= 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Triple indirect block num <= 0.\n", GET_DBLOCK_FROM_IBLOCK);
        return -1;
    }
    
    ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);
    data_block_num = double_indirect_block_arr[double_i_idx];
    altfs_free_memory(double_indirect_block_arr);
    
    if(data_block_num<=0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Double indirect block num from triple <= 0.\n", GET_DBLOCK_FROM_IBLOCK);
        return -1;
    }
    *prev_indirect_block = data_block_num;

    ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);
    data_block_num = single_indirect_block_arr[inner_idx];
    altfs_free_memory(single_indirect_block_arr);

    fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from triple indirect block\n", GET_DBLOCK_FROM_IBLOCK, data_block_num);
    return data_block_num;
}

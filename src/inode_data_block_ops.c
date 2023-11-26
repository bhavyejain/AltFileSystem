#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<stdio.h>
#include<fuse.h>
#include "../header/superblock_layer.h"
#include "../header/disk_layer.h"
#include "../header/inode_ops.h"
#include "../header/data_block_ops.h"
#include "../header/initialize_fs_ops.h"
#include "../header/inode_data_block_ops.h"


bool add_datablock_to_inode(struct inode* inodeObj, const ssize_t data_block_num)
{
    // Add data block to inode after incrementing num of blocks in inode struct
    ssize_t logical_block_num = inodeObj->i_blocks_num;

    // Follow same structure of translating to data blocks as in initialize_fs_ops
    if(logical_block_num < NUM_OF_DIRECT_BLOCKS){
        inodeObj->i_direct_blocks[logical_block_num] = data_block_num;
    }
    // TODO: This is repetitive code coming up in lot of places. See if this can be made modular
    else if(logical_block_num < DIRECT_PLUS_SINGLE_INDIRECT_ADDR){
        if(logical_block_num == NUM_OF_DIRECT_BLOCKS)
        {
            // If data block == 12 => new single indirect block needs to be added
            ssize_t single_indirect_block_num = allocate_data_block();
            if(single_indirect_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for single indirect data block. Exiting\n", ADD_DATABLOCK_TO_INODE);
                return false;
            }
            inodeObj->i_single_indirect = single_indirect_block_num;
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully allocated new data block for new single indirect block\n", ADD_DATABLOCK_TO_INODE);
        }

        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_single_indirect);
        single_indirect_block_arr[logical_block_num - NUM_OF_DIRECT_BLOCKS] = data_block_num;
        if(!write_data_block(inodeObj->i_single_indirect, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to single indirect block\n", ADD_DATABLOCK_TO_INODE);
            return false;
        }
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added data block in single indirect block\n", ADD_DATABLOCK_TO_INODE);
        altfs_free_memory(single_indirect_block_arr);
    }
    else if(logical_block_num < DIRECT_PLUS_SINGLE_DOUBLE_INDIRECT_ADDR)
    {
        // if file block num == 12 + 512 => need a new double indirect block
        if(logical_block_num == DIRECT_PLUS_SINGLE_INDIRECT_ADDR)
        {
            ssize_t double_indirect_data_block_num = allocate_data_block();
            if(double_indirect_data_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for double indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            inodeObj->i_double_indirect = double_indirect_data_block_num;
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully allocated new data block for new double indirect block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
        }
        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_double_indirect);
        ssize_t offset = logical_block_num - DIRECT_PLUS_SINGLE_INDIRECT_ADDR;
        // This is to create the first indirect block in the single indirect block
        if(offset % NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR == 0){
            ssize_t single_indirect_block_num = allocate_data_block();
            if(single_indirect_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for single indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            double_indirect_block_arr[offset/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR] = single_indirect_block_num;
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added data block in single indirect block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
            if(!write_data_block(inodeObj->i_double_indirect, (char*)double_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to double indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
        }
        ssize_t single_indirect_block_num = double_indirect_block_arr[offset/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR];
        if(single_indirect_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Invalid blocknum for single indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(single_indirect_block_num);
        single_indirect_block_arr[offset % NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR] = data_block_num;
        if(!write_data_block(single_indirect_block_num, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to single indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }
        altfs_free_memory(double_indirect_block_arr);
        altfs_free_memory(single_indirect_block_arr);
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added double indirect data block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
    }
    else
    {
        if(logical_block_num == DIRECT_PLUS_SINGLE_DOUBLE_INDIRECT_ADDR){
            ssize_t triple_data_block_num = allocate_data_block();
            if(triple_data_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for triple indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            inodeObj->i_triple_indirect = triple_data_block_num;
        }
        ssize_t* triple_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_triple_indirect);
        ssize_t triple_logical_block_num = logical_block_num - DIRECT_PLUS_SINGLE_DOUBLE_INDIRECT_ADDR;
        ssize_t triple_fblock_offset = triple_logical_block_num/NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;
        if(triple_logical_block_num % NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR == 0){
            ssize_t double_indirect_data_block_num = allocate_data_block();
            if(double_indirect_data_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for double indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            triple_indirect_block_arr[triple_fblock_offset] = double_indirect_data_block_num;
            if(!write_data_block(inodeObj->i_triple_indirect, (char *)triple_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to triple indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
        }
        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(triple_indirect_block_arr[triple_fblock_offset]);
        if(triple_logical_block_num % NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR == 0){
            ssize_t single_indirect_block_num = allocate_data_block();
            if(single_indirect_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate block for single indirect block for file block number %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            double_indirect_block_arr[(triple_logical_block_num/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)%NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR] = single_indirect_block_num;
            if(!write_data_block(triple_indirect_block_arr[triple_fblock_offset], (char*)double_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to double indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
        }
        ssize_t double_fblock_offset = (triple_logical_block_num/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)%NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;
        ssize_t single_indirect_block_num = double_indirect_block_arr[double_fblock_offset];
        if(single_indirect_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Invalid block num for single indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(single_indirect_block_num);
        single_indirect_block_arr[triple_logical_block_num % NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR] = data_block_num;
        if(!write_data_block(single_indirect_block_num, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to write to single indirect block for file bloxk num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }
        altfs_free_memory(triple_indirect_block_arr);
        altfs_free_memory(double_indirect_block_arr);
        altfs_free_memory(single_indirect_block_arr);
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added triple indirect data block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
    }
    inodeObj->i_blocks_num++;
    return true;
}

bool overwrite_datablock_to_inode(struct inode *inodeObj, ssize_t logical_block_num, ssize_t data_block_num, ssize_t *prev_indirect_block)
{
    if (logical_block_num > inodeObj->i_blocks_num)
    {
        fuse_log(FUSE_LOG_ERR, "%s : file block num %zd is greater than number of blocks in inode\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
        return false;
    }

    // If file block is within direct block count, return data block number directly
    if(logical_block_num < NUM_OF_DIRECT_BLOCKS){
        inodeObj->i_direct_blocks[logical_block_num] = data_block_num;
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully overwrote logical block %zd with data block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num, data_block_num);
        return true;
    }

    // Adjust logical block number for single indirect
    logical_block_num -= NUM_OF_DIRECT_BLOCKS;

     // If file block num < 512 => single indirect block
    if(logical_block_num < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)
    {
        if(inodeObj->i_single_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Single indirect is set to 0 for inode for logical block num %zd.\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }

        // Read single indirect block and extract data block num from logical block num
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_single_indirect);
        single_indirect_block_arr[logical_block_num] = data_block_num;

        if (!write_data_block(inodeObj->i_single_indirect, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Writing to single indirect block failed for logical block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }
        altfs_free_memory(single_indirect_block_arr);
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully overwrote logical block %zd with data block %zd from single indirect block\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num, data_block_num);
        return true;
    }

    // Adjust logical block number for double indirect
    logical_block_num -= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;

    // If file block num < 512*512 => double indirect block
    if(logical_block_num < NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR)
    {
        if(inodeObj->i_double_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Double indirect is set to 0 for inode for logical block num %zd.\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }

        ssize_t double_i_idx = logical_block_num / NUM_OF_ADDRESSES_PER_BLOCK;
        ssize_t inner_idx = logical_block_num % NUM_OF_ADDRESSES_PER_BLOCK;

        if(inner_idx != 0 && *prev_indirect_block != 0)
        {
            ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(*prev_indirect_block);
            single_indirect_block_arr[inner_idx] = data_block_num;

            if (!write_data_block(single_data_block_num, (char*)single_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Writing to single double block failed for file block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            altfs_free_memory(single_indirect_block_arr);
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully overwrote logical block %zd with data block %zd from double indirect block using cached indirect block value %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num, data_block_num, *prev_indirect_block);
            return true;
        }

        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(inode->double_indirect);
        ssize_t single_data_block_num = double_indirect_block_arr[double_i_idx];
        altfs_free_memory(double_indirect_block_arr);
        
        if(single_data_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Single indirect < 0 for file block num %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }

        *prev_indirect_block = single_data_block_num;
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(single_data_block_num);
        single_indirect_block_arr[inner_idx] = data_block_num;

        if (!write_data_block(single_data_block_num, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Writing to single indirect block failed for file block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }
        altfs_free_memory(single_indirect_block_arr);
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully overwrote logical block %zd with data block %zd from double indirect.\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num, data_block_num);
        return true;
    }

    // Adjust logical block number for triple indirect
    logical_block_num -= NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;

    // If file block num < 512*512*512 => triple indirect block
    if(inodeObj->i_triple_indirect == 0)
    {
        fuse_log(FUSE_LOG_ERR,"%s : Triple indirect block is set to 0 for inode. Exiting\n", OVERWRITE_DATABLOCK_TO_INODE);
        return false;
    }

    ssize_t triple_i_idx = logical_block_num / NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;
    ssize_t double_i_idx = (logical_block_num / NUM_OF_ADDRESSES_PER_BLOCK) % NUM_OF_ADDRESSES_PER_BLOCK;
    ssize_t inner_idx = logical_block_num % NUM_OF_ADDRESSES_PER_BLOCK;

    if(inner_idx != 0 && *prev_indirect_block != 0)
    {
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(*prev_indirect_block);
        single_indirect_block_arr[inner_idx] = data_block_num;
        
        if (!write_data_block(single_data_block_num, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Writing to triple indirect block failed for file block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }
        altfs_free_memory(single_indirect_block_arr);
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully overwrote logical block %zd with data block %zd from triple indirect block using cached indirect block value %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num, data_block_num, *prev_indirect_block);
        return true;
    }

    ssize_t* triple_indirect_block_arr = (ssize_t*) read_data_block(inode->triple_indirect);
    ssize_t double_data_block_num = triple_indirect_block_arr[triple_i_idx];
    altfs_free_memory(triple_indirect_block_arr);
    
    if(double_data_block_num <= 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Triple indirect block num <= 0.\n", OVERWRITE_DATABLOCK_TO_INODE);
        return false;
    }

    ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(double_data_block_num);
    ssize_t single_data_block_num = double_indirect_block_arr[double_i_idx];
    altfs_free_memory(double_indirect_block_arr);
    
    if(single_data_block_num <= 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Double indirect block num <= 0.\n", OVERWRITE_DATABLOCK_TO_INODE);
        return false;
    }

    ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(single_data_block_num);
    single_indirect_block_arr[inner_idx] = data_block_num;
    
    if (!write_data_block(single_data_block_num, (char*)single_indirect_block_arr))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Writing to triple indirect block failed for file block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
        return false;
    }

    altfs_free_memory(single_indirect_block_arr);
    fuse_log(FUSE_LOG_DEBUG, "%s : Successfully overwrote logical block %zd with data block %zd from triple indirect.\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num, data_block_num);
    return true;
}

// TODO: Finish this code and refactor entire file
bool remove_datablocks_from_inode(struct inode* inodeObj, const ssize_t logical_block_num)
{
    if (logical_block_num >= inodeObj->i_blocks_num)
    {
        fuse_log(FUSE_LOG_ERR, "%s : file block number is greater than number of blocks in inode \n",)
    }
    return true;
}
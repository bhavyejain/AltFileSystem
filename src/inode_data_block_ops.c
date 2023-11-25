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
    ssize_t file_block_num = inodeObj->i_blocks_num;

    // Follow same structure of translating to data blocks as in initialize_fs_ops
    if(file_block_num < NUM_OF_DIRECT_BLOCKS){
        inodeObj->i_direct_blocks[file_block_num] = data_block_num;
    }
    // TODO: This is repetitive code coming up in lot of places. See if this can be made modular
    else if(file_block_num < DIRECT_PLUS_SINGLE_INDIRECT_ADDR){
        if(file_block_num == NUM_OF_DIRECT_BLOCKS)
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
        single_indirect_block_arr[file_block_num - NUM_OF_DIRECT_BLOCKS] = data_block_num;
        if(!write_data_block(inodeObj->i_single_indirect, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to single indirect block\n", ADD_DATABLOCK_TO_INODE);
            return false;
        }
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added data block in single indirect block\n", ADD_DATABLOCK_TO_INODE);
        altfs_free_memory(single_indirect_block_arr);
    }
    else if(file_block_num < DIRECT_PLUS_SINGLE_DOUBLE_INDIRECT_ADDR)
    {
        // if file block num == 12 + 512 => need a new double indirect block
        if(file_block_num == DIRECT_PLUS_SINGLE_INDIRECT_ADDR)
        {
            ssize_t double_indirect_data_block_num = allocate_data_block();
            if(double_indirect_data_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for double indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
                return false;
            }
            inodeObj->i_double_indirect = double_indirect_data_block_num;
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully allocated new data block for new double indirect block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
        }
        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_double_indirect);
        ssize_t offset = file_block_num - DIRECT_PLUS_SINGLE_INDIRECT_ADDR;
        // This is to create the first indirect block in the single indirect block
        if(offset % NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR == 0){
            ssize_t single_indirect_block_num = allocate_data_block();
            if(single_indirect_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for single indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
                return false;
            }
            double_indirect_block_arr[offset/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR] = single_indirect_block_num;
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added data block in single indirect block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
            if(!write_data_block(inodeObj->i_double_indirect, (char*)double_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to double indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
                return false;
            }
        }
        ssize_t single_indirect_block_num = double_indirect_block_arr[offset/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR];
        if(single_indirect_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Invalid blocknum for single indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
            return false;
        }
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(single_indirect_block_num);
        single_indirect_block_arr[offset % NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR] = data_block_num;
        if(!write_data_block(single_indirect_block_num, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to single indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
            return false;
        }
        altfs_free_memory(double_indirect_block_arr);
        altfs_free_memory(single_indirect_block_arr);
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added double indirect data block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
    }
    else
    {
        if(file_block_num == DIRECT_PLUS_SINGLE_DOUBLE_INDIRECT_ADDR){
            ssize_t triple_data_block_num = allocate_data_block();
            if(triple_data_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for triple indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
                return false;
            }
            inodeObj->i_triple_indirect = triple_data_block_num;
        }
        ssize_t* triple_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_triple_indirect);
        ssize_t triple_file_block_num = file_block_num - DIRECT_PLUS_SINGLE_DOUBLE_INDIRECT_ADDR;
        ssize_t triple_fblock_offset = triple_file_block_num/NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;
        if(triple_file_block_num % NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR == 0){
            ssize_t double_indirect_data_block_num = allocate_data_block();
            if(double_indirect_data_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for double indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
                return false;
            }
            triple_indirect_block_arr[triple_fblock_offset] = double_indirect_data_block_num;
            if(!write_data_block(inodeObj->i_triple_indirect, (char *)triple_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to triple indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
                return false;
            }
        }
        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(triple_indirect_block_arr[triple_fblock_offset]);
        if(triple_file_block_num % NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR == 0){
            ssize_t single_indirect_block_num = allocate_data_block();
            if(single_indirect_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate block for single indirect block for file block number %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
                return false;
            }
            double_indirect_block_arr[(triple_file_block_num/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)%NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR] = single_indirect_block_num;
            if(!write_data_block(triple_indirect_block_arr[triple_fblock_offset], (char*)double_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to double indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
                return false;
            }
        }
        ssize_t double_fblock_offset = (triple_file_block_num/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)%NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;
        ssize_t single_indirect_block_num = double_indirect_block_arr[double_fblock_offset];
        if(single_indirect_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Invalid block num for single indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
            return false;
        }
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(single_indirect_block_num);
        single_indirect_block_arr[triple_file_block_num % NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR] = data_block_num;
        if(!write_data_block(single_indirect_block_num, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to write to single indirect block for file bloxk num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
            return false;
        }
        altfs_free_memory(triple_indirect_block_arr);
        altfs_free_memory(double_indirect_block_arr);
        altfs_free_memory(single_indirect_block_arr);
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added triple indirect data block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, file_block_num);
    }
    inodeObj->i_blocks_num++;
    return true;
}

bool overwrite_datablock_to_inode(struct inode *inodeObj, ssize_t file_block_num, ssize_t data_block_num)
{
    if (file_block_num > inodeObj->i_blocks_num)
    {
        fuse_log(FUSE_LOG_ERR, "%s : file block num %zd is greater than number of blocks in inode\n", OVERWRITE_DATABLOCK_TO_INODE, file_block_num);
        return false;
    }
    if(file_block_num < NUM_OF_DIRECT_BLOCKS){
        inodeObj->i_direct_blocks[file_block_num] = data_block_num;
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully overwrote fileblock %zd with data block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, file_block_num, data_block_num);
    }
    else if(file_block_num < DIRECT_PLUS_SINGLE_INDIRECT_ADDR){
        if(inodeObj->i_single_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Single indirect is set to 0 for inode for file block num %zd. Exiting\n", OVERWRITE_DATABLOCK_TO_INODE, file_block_num);
            return false;
        }
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_single_indirect);
        single_indirect_block_arr[file_block_num - NUM_OF_DIRECT_BLOCKS] = data_block_num;

        if (!write_data_block(inodeObj->i_single_indirect, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Writing to single indirect block failed for file block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, file_block_num);
            return false;
        }
        altfs_free_memory(single_indirect_block_arr);
    }
    else if(file_block_num < DIRECT_PLUS_SINGLE_DOUBLE_INDIRECT_ADDR){
        if(inodeObj->i_double_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Double indirect is set to 0 for inode for file block num %zd. Exiting\n", OVERWRITE_DATABLOCK_TO_INODE, file_block_num);
            return false;
        }
        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(inode->double_indirect);
        ssize_t offset = file_block_num - DIRECT_PLUS_SINGLE_INDIRECT_ADDR;
        ssize_t single_data_block_num = double_indirect_block_arr[offset/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR];
        altfs_free_memory(double_indirect_block_arr);
        
        if(single_data_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Single indirect < 0 for file block num %zd\n", OVERWRITE_DATABLOCK_TO_INODE, file_block_num);
            return false;
        }
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(single_data_block_num);
        single_indirect_block_arr[offset % NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR] = data_block_num;

        if (!write_data_block(single_data_block_num, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Writing to single indirect block failed for file block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, file_block_num);
            return false;
        }
        altfs_free_memory(single_indirect_block_arr);
    }
    else{
        if(inodeObj->i_triple_indirect == 0){
            return false;
        }
        ssize_t* triple_indirect_block_arr = (ssize_t*) read_data_block(inode->triple_indirect);
        ssize_t offset = file_block_num - DIRECT_PLUS_SINGLE_DOUBLE_INDIRECT_ADDR;
        ssize_t double_data_block_num = triple_indirect_block_arr[offset/NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR];
        altfs_free_memory(triple_indirect_block_arr);
        
        if(double_data_block_num <= 0){
            return false;
        }

        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(double_data_block_num);
        ssize_t single_data_block_num = double_indirect_block_arr[(offset/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)%NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR];
        altfs_free_memory(double_indirect_block_arr);
        if(single_data_block_num <= 0)
        {
            return false;
        }
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(single_data_block_num);
        single_indirect_block_arr[offset%NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR] = data_block_num;
        
        if (!write_data_block(single_data_block_num, (char*)single_indirect_block_arr))
            return false;
        altfs_free_memory(single_indirect_block_arr);
    }
    return true;
}

bool remove_datablocks_from_inode(struct inode* inodeObj, const ssize_t file_block_num)
{
    if (file_block_num >= inodeObj->i_blocks_num)
    {
        fuse_log(FUSE_LOG_ERR, "%s : file block number is greater than number of blocks in inode \n",)
    }
    return true;
}
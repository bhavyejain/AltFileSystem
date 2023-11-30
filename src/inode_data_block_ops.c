#include "../header/superblock_layer.h"
#include "../header/inode_ops.h"
#include "../header/data_block_ops.h"
#include "../header/inode_data_block_ops.h"


bool add_datablock_to_inode(struct inode *inodeObj, const ssize_t data_block_num)
{
    // Add data block to inode after incrementing num of blocks in inode struct
    ssize_t logical_block_num = inodeObj->i_blocks_num;

    // Follow same structure of translating to data blocks as in directory_ops
    if(logical_block_num < NUM_OF_DIRECT_BLOCKS){
        inodeObj->i_direct_blocks[logical_block_num] = data_block_num;
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added data block to direct block\n", ADD_DATABLOCK_TO_INODE);
        inodeObj->i_blocks_num++;
        return true;
    }

    // Adjust logical block number for single indirect
    logical_block_num -= NUM_OF_DIRECT_BLOCKS;

    if(logical_block_num < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR){
        // If data block == 12 => new single indirect block needs to be added
        if(logical_block_num == 0)
        {
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
        single_indirect_block_arr[logical_block_num] = data_block_num;
        
        if(!write_data_block(inodeObj->i_single_indirect, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to single indirect block\n", ADD_DATABLOCK_TO_INODE);
            return false;
        }

        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added data block in single indirect block\n", ADD_DATABLOCK_TO_INODE);
        altfs_free_memory(single_indirect_block_arr);
        inodeObj->i_blocks_num++;
        return true;
    }

    // Adjust logical block number for double indirect
    logical_block_num -= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;

    if(logical_block_num < NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR)
    {
        // if file block num == 12 + 512 => need a new double indirect block
        if(logical_block_num == 0)
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

        ssize_t double_i_idx = logical_block_num / NUM_OF_ADDRESSES_PER_BLOCK;
        ssize_t inner_idx = logical_block_num % NUM_OF_ADDRESSES_PER_BLOCK;

        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_double_indirect);

        // This is to create the first indirect block in the single indirect block
        if(inner_idx == 0)
        { 
            ssize_t single_indirect_block_num = allocate_data_block();
            if(single_indirect_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for single indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            
            double_indirect_block_arr[double_i_idx] = single_indirect_block_num;
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added data block in single indirect block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
            
            if(!write_data_block(inodeObj->i_double_indirect, (char*)double_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to double indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
        }
        
        ssize_t single_indirect_block_num = double_indirect_block_arr[double_i_idx];
        altfs_free_memory(double_indirect_block_arr);
        
        if(single_indirect_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Invalid blocknum for single indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }

        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(single_indirect_block_num);
        single_indirect_block_arr[inner_idx] = data_block_num;
        
        if(!write_data_block(single_indirect_block_num, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to single indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }
        
        altfs_free_memory(single_indirect_block_arr);
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added double indirect data block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
        inodeObj->i_blocks_num++;
        return true;
    }

    // Adjust logical block number for triple indirect
    logical_block_num -= NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;
    
    if (logical_block_num < NUM_OF_TRIPLE_INDIRECT_BLOCK_ADDR)
    {
        if(logical_block_num == 0){
            ssize_t triple_data_block_num = allocate_data_block();
            if(triple_data_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for triple indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            inodeObj->i_triple_indirect = triple_data_block_num;
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully allocated new data block for new triple indirect block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
        }

        ssize_t* triple_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_triple_indirect);

        ssize_t triple_i_idx = logical_block_num / NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;
        ssize_t double_i_idx = (logical_block_num / NUM_OF_ADDRESSES_PER_BLOCK) % NUM_OF_ADDRESSES_PER_BLOCK;
        ssize_t inner_idx = logical_block_num % NUM_OF_ADDRESSES_PER_BLOCK;
        
        if(logical_block_num % NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR == 0){
            ssize_t double_indirect_data_block_num = allocate_data_block();
            
            if(double_indirect_data_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate new data block for double indirect data block with file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            
            triple_indirect_block_arr[triple_i_idx] = double_indirect_data_block_num;
            
            if(!write_data_block(inodeObj->i_triple_indirect, (char *)triple_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to triple indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
        }
        
        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(triple_indirect_block_arr[triple_i_idx]);
        
        if(inner_idx == 0){
            ssize_t single_indirect_block_num = allocate_data_block();
            if(single_indirect_block_num == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to allocate block for single indirect block for file block number %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            
            double_indirect_block_arr[double_i_idx] = single_indirect_block_num;
            
            if(!write_data_block(triple_indirect_block_arr[triple_i_idx], (char*)double_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to write data to double indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
        }
        altfs_free_memory(triple_indirect_block_arr);

        ssize_t single_indirect_block_num = double_indirect_block_arr[double_i_idx];
        altfs_free_memory(double_indirect_block_arr);
        
        if(single_indirect_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Invalid block num for single indirect block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }

        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(single_indirect_block_num);
        single_indirect_block_arr[inner_idx] = data_block_num;

        if(!write_data_block(single_indirect_block_num, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to write to single indirect block for file bloxk num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }
        altfs_free_memory(single_indirect_block_arr);
        inodeObj->i_blocks_num++;
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully added triple indirect data block for file block num %zd\n", ADD_DATABLOCK_TO_INODE, logical_block_num);
        return true;
    }
    fuse_log(FUSE_LOG_ERR, "%s : Reached end of function without going into any conditions\n", ADD_DATABLOCK_TO_INODE);
    return false;
}

bool overwrite_datablock_to_inode(struct inode *inodeObj, ssize_t logical_block_num, ssize_t data_block_num, ssize_t *prev_indirect_block)
{
    if (logical_block_num > inodeObj->i_blocks_num)
    {
        fuse_log(FUSE_LOG_ERR, "%s : file block num %zd is greater than number of blocks in inode\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
        return false;
    }

    // If file block is within direct block count, return data block number directly
    if(logical_block_num < NUM_OF_DIRECT_BLOCKS)
    {
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

            if (!write_data_block(data_block_num, (char*)single_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Writing to single double block failed for file block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
                return false;
            }
            altfs_free_memory(single_indirect_block_arr);
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully overwrote logical block %zd with data block %zd from double indirect block using cached indirect block value %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num, data_block_num, *prev_indirect_block);
            return true;
        }

        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_double_indirect);
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
        
        if (!write_data_block(data_block_num, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Writing to triple indirect block failed for file block %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num);
            return false;
        }
        altfs_free_memory(single_indirect_block_arr);
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully overwrote logical block %zd with data block %zd from triple indirect block using cached indirect block value %zd\n", OVERWRITE_DATABLOCK_TO_INODE, logical_block_num, data_block_num, *prev_indirect_block);
        return true;
    }

    ssize_t* triple_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_triple_indirect);
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

bool remove_datablocks_utility(struct inode* inodeObj, ssize_t p_block_num, ssize_t indirectionlevel)
{
    if (p_block_num <= 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Invalid data block number for removal with indirection level %d", REMOVE_DATABLOCKS_UTILITY, indirectionlevel);
        return false;
    }

    ssize_t *buffer = (ssize_t*) read_data_block(p_block_num);
    for(ssize_t i = 0; i < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR; i++)
    {
        // we have reached the end of the data in the block 
        if (buffer[i] == 0)
            break;
        switch (indirectionlevel)
        {
            case 1:
                free_data_block(buffer[i]);
                break;
            default: 
                remove_datablocks_utility(inodeObj, buffer[i], indirectionlevel-1);
                break;
        }
    }
    free_data_block(p_block_num);
    altfs_free_memory(buffer);
    return true;
}

bool remove_datablocks_from_inode(struct inode* inodeObj, ssize_t logical_block_num)
{
    // Equality is required here because logical blocks are 0-indexed.
    // Example - if i_blocks_num = 8 => there are logical blocks from 0-7
    if (logical_block_num >= inodeObj->i_blocks_num)
    {
        fuse_log(FUSE_LOG_ERR, "%s : file block number is greater than number of blocks in inode \n", REMOVE_DATABLOCKS_FROM_INODE);
        return false;
    }

    ssize_t ending_block_num = inodeObj->i_blocks_num;
    ssize_t starting_block_num = logical_block_num;
    
    // <= operator is used here since the same code needs to run when logical_block_num = 12 i.e first single indirect block
    // In that case node->i_single_indirect = 0 after removal which will be done by the code below
    // If not, it will go to the next condition where there is no special handling to set node->i_single_indirect = 0
    // The same applies <= operator used for single, double and triple indirect conditions as well
    if (logical_block_num <= NUM_OF_DIRECT_BLOCKS)
    {
        for (ssize_t i = logical_block_num; i < NUM_OF_DIRECT_BLOCKS && i < ending_block_num; i++)
        {
            if (!free_data_block(inodeObj->i_direct_blocks[i]))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to free direct block %zd starting deletion from logical block %zd\n", REMOVE_DATABLOCKS_FROM_INODE, i, logical_block_num);
                return false;
            }
            inodeObj->i_direct_blocks[i] = 0;
        }

        // <= is used here since ending_block_num is initialized to number of blocks allocated for the inode
        if (ending_block_num <= NUM_OF_DIRECT_BLOCKS)
        {
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully deleted data blocks from block %zd to %zd\n",REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            inodeObj->i_blocks_num = starting_block_num;
            return true;
        }

        if (!remove_datablocks_utility(inodeObj, inodeObj->i_single_indirect, 1))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to free single indirect blocks from block %zd to %zd\n", REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            return false;
        }
        inodeObj->i_single_indirect = 0;
        // Adjusting for single indirect block
        ending_block_num -= NUM_OF_DIRECT_BLOCKS;

        if (ending_block_num <= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)
        {
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully deleted data blocks from block %zd to %zd\n",REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            inodeObj->i_blocks_num = starting_block_num;
            return true;
        }

        if (!remove_datablocks_utility(inodeObj, inodeObj->i_double_indirect, 2))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to free double indirect blocks from block %zd to %zd\n", REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            return false;
        }
        inodeObj->i_double_indirect = 0;
        // Adjusting for double indirect block
        ending_block_num -= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;

        if (ending_block_num <= NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR)
        {
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully deleted data blocks from block %zd to %zd\n",REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            inodeObj->i_blocks_num = starting_block_num;
            return true;
        }

        if (!remove_datablocks_utility(inodeObj, inodeObj->i_triple_indirect, 3))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to free triple indirect blocks from block %zd to %zd\n", REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            return false;
        }
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully deleted data blocks from block %zd to %zd\n",REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
        inodeObj->i_triple_indirect = 0;
        inodeObj->i_blocks_num = starting_block_num;
        return true;
    }

    // Adjusting for single indirect blocks
    logical_block_num -= NUM_OF_DIRECT_BLOCKS;
    
    if (logical_block_num <= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)
    {
        if(inodeObj->i_single_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR,"%s : Single indirect block is set to 0 for inode.\n", REMOVE_DATABLOCKS_FROM_INODE);
            return false;
        }

        // remove all nodes starting from the given logical num
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_single_indirect);
        // Adjusting for single indirect block
        ending_block_num -= NUM_OF_DIRECT_BLOCKS;

        for(ssize_t i = logical_block_num; i < ending_block_num && i < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR; i++)
        {
            if (!free_data_block(single_indirect_block_arr[i]))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to free single indirect block %zd starting deletion from logical block %zd\n", REMOVE_DATABLOCKS_FROM_INODE, i, logical_block_num);
                return false;
            }
            single_indirect_block_arr[i] = 0; // To ensure the i_single_indirect is updated based on free nodes
        }

        if(!write_data_block(inodeObj->i_single_indirect, (char*)single_indirect_block_arr))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to write modified single indirect block %zd \n", REMOVE_DATABLOCKS_FROM_INODE, inodeObj->i_single_indirect);
            return false;
        }

        if (ending_block_num <= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)
        {
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully deleted data blocks from block %zd to %zd\n",REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            inodeObj->i_blocks_num = starting_block_num;
            return true;
        }

        if (!remove_datablocks_utility(inodeObj, inodeObj->i_double_indirect, 2))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to free double indirect blocks from block %zd to %zd\n", REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            return false;
        }
        inodeObj->i_double_indirect = 0;
        // Adjusting for double indirect block
        ending_block_num -= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;

        if (ending_block_num <= NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR)
        {
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully deleted data blocks from block %zd to %zd\n",REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            inodeObj->i_blocks_num = starting_block_num;
            return true;
        }

        if (!remove_datablocks_utility(inodeObj, inodeObj->i_triple_indirect, 3))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to free triple indirect blocks from block %zd to %zd\n", REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            return false;
        }
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully deleted data blocks from block %zd to %zd\n",REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
        inodeObj->i_triple_indirect = 0;
        inodeObj->i_blocks_num = starting_block_num;
        return true;
    }

    // Adjusting for double indirect blocks
    logical_block_num -= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;

    if (logical_block_num <= NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR)
    {
        if(inodeObj->i_double_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR,"%s : Double indirect block is set to 0 for inode.\n", REMOVE_DATABLOCKS_FROM_INODE);
            return false;
        }

        ssize_t double_i_idx = logical_block_num / NUM_OF_ADDRESSES_PER_BLOCK;
        ssize_t inner_idx = logical_block_num % NUM_OF_ADDRESSES_PER_BLOCK;

        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_double_indirect);

        // Adjusting for double indirect block
        ending_block_num -= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR + NUM_OF_DIRECT_BLOCKS;

        for(ssize_t i = double_i_idx; i < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR; i++)
        {
            ssize_t j = (i == double_i_idx) ? inner_idx : 0;

            ssize_t data_block_num = double_indirect_block_arr[i];
            if(data_block_num <= 0)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Double indirect block num <= 0.\n", REMOVE_DATABLOCKS_FROM_INODE);
                return false;
            }

            ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);

            for(;j < ending_block_num && j < NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR; j++) 
            {
                if (!free_data_block(single_indirect_block_arr[j]))
                {
                    fuse_log(FUSE_LOG_ERR, "%s : Failed to free block %zd \n", REMOVE_DATABLOCKS_FROM_INODE, single_indirect_block_arr[j]);
                    return false;
                }
                single_indirect_block_arr[j] = 0; // To ensure the i_double_indirect's indirect block is updated based on free nodes
            }

            if(!write_data_block(data_block_num, (char*)single_indirect_block_arr))
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to write modified double indirect block %zd \n", REMOVE_DATABLOCKS_FROM_INODE, inodeObj->i_single_indirect);
                return false;
            }

            altfs_free_memory(single_indirect_block_arr);
            
            // if ending_block_num is reached, break out of loop
            if (j == ending_block_num)
                break;

            // Suppose ending_block_num was 520, after first iteration, 512 blocks are removed
            ending_block_num -= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;

            // We should not free the data block if there are elements in it
            // This can happen in the first block we are starting the deletion from
            // For example - Delete from block 526 => delete from logical number 2 onwards
            // The single indirect corresponding to this should not be freed
            if (!(i == double_i_idx && j > 0))
            {
                if (!free_data_block(data_block_num))
                {
                    fuse_log(FUSE_LOG_ERR, "%s : Failed to free block %zd \n", REMOVE_DATABLOCKS_FROM_INODE, data_block_num);
                    return false;
                }
            }
        }
        altfs_free_memory(double_indirect_block_arr);

        if (ending_block_num <= NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR)
        {
            fuse_log(FUSE_LOG_DEBUG, "%s : Successfully deleted data blocks from block %zd to %zd\n",REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            inodeObj->i_blocks_num = starting_block_num;
            return true;
        }        
        
        if (!remove_datablocks_utility(inodeObj, inodeObj->i_triple_indirect, 3))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to free triple indirect blocks from block %zd to %zd\n", REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
            return false;
        }
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully deleted data blocks from block %zd to %zd\n",REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
        inodeObj->i_triple_indirect = 0;
        inodeObj->i_blocks_num = starting_block_num;
        return true;
    }

    logical_block_num -= NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;

    // TODO: This code also definitely has bugs. Make resolutions similar to commits:
    // f808eb67d7e1a38f3fc4f55b269ac724ca75b12a
    // 455cfee87622186327d5c9b97dd7b01eaff62c6e
    // fe85c30d344c5fe84cfef138413b270af95d5b2c
    if (logical_block_num <= NUM_OF_TRIPLE_INDIRECT_BLOCK_ADDR)
    {
        if(inodeObj->i_triple_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR,"%s : Triple indirect block is set to 0 for inode.\n", GET_DBLOCK_FROM_IBLOCK);
            return false;
        }

        ssize_t triple_i_idx = logical_block_num / NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;
        ssize_t double_i_idx = (logical_block_num / NUM_OF_ADDRESSES_PER_BLOCK) % NUM_OF_ADDRESSES_PER_BLOCK;
        ssize_t inner_idx = logical_block_num % NUM_OF_ADDRESSES_PER_BLOCK;

        ssize_t* triple_indirect_block_arr = (ssize_t*) read_data_block(inodeObj->i_triple_indirect);

        // Adjusting for triple indirect block
        ending_block_num -= NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR + NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR + NUM_OF_DIRECT_BLOCKS;

        for(ssize_t i = triple_i_idx; i < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR; i++)
        {
            ssize_t data_block_num = triple_indirect_block_arr[i];
            if(data_block_num <= 0)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Triple indirect block num <= 0.\n", REMOVE_DATABLOCKS_FROM_INODE);
                return false;
            }

            ssize_t j = (i == triple_i_idx) ? double_i_idx : 0;
            ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);

            for(;j < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR; j++)
            {
                ssize_t double_indirect_data_block_num = double_indirect_block_arr[j];
                if(double_indirect_data_block_num <= 0)
                {
                    fuse_log(FUSE_LOG_ERR, "%s : Double indirect block num from triple <= 0.\n", REMOVE_DATABLOCKS_FROM_INODE);
                    return false;
                }
                ssize_t k = (j == double_i_idx) ? inner_idx : 0;
                ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(double_indirect_data_block_num);
                for(; k < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR && k < ending_block_num; k++) // TODO: Check if the bounds checking for i,j,k, are right
                {
                    if (!free_data_block(single_indirect_block_arr[k]))
                    {
                        fuse_log(FUSE_LOG_ERR, "%s : Failed to free block %zd \n", REMOVE_DATABLOCKS_FROM_INODE, single_indirect_block_arr[k]);
                        return false;
                    }
                    single_indirect_block_arr[k] = 0; // To ensure the i_triple_indirect's indirect block is updated based on free nodes
                }

                if(!write_data_block(double_indirect_data_block_num, (char*)single_indirect_block_arr))
                {
                    fuse_log(FUSE_LOG_ERR, "%s : Failed to write modified triple indirect block %zd \n", REMOVE_DATABLOCKS_FROM_INODE, inodeObj->i_single_indirect);
                    return false;
                }

                altfs_free_memory(single_indirect_block_arr);
            }
            altfs_free_memory(double_indirect_block_arr);
        }
        altfs_free_memory(triple_indirect_block_arr);
        
        fuse_log(FUSE_LOG_DEBUG, "%s : Successfully deleted data blocks from block %zd to %zd\n",REMOVE_DATABLOCKS_FROM_INODE, starting_block_num, inodeObj->i_blocks_num);
        inodeObj->i_triple_indirect = 0;
        inodeObj->i_blocks_num = starting_block_num;
        return true;
    }
    return false;
}
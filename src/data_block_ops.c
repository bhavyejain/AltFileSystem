#include "../header/data_block_ops.h"
#include "../header/disk_layer.h"
#include "../header/superblock_layer.h"

ssize_t allocate_data_block()
{
    if(altfs_superblock->s_freelist_head == 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s All data blocks allocated!\n", ALLOCATE_DATA_BLOCK);
        return -1;
    }

    char buffer[BLOCK_SIZE];
    if(!altfs_read_block(altfs_superblock->s_freelist_head, buffer))
    {
        fuse_log(FUSE_LOG_ERR, "%s Bata block not found at address.\n", ALLOCATE_DATA_BLOCK);
        return -1;
    }

    ssize_t* data_block_numbers = (ssize_t*)buffer;
    ssize_t allocated_data_block_number = 0;

    // TODO: Possible optimization in policy of allocating new blocks
    // starts from 1 because 0 is used to indicate next free node
    for(ssize_t i = 1; i < NUM_OF_ADDRESSES_PER_BLOCK; ++i)
    {
        ssize_t data_block_number = data_block_numbers[i];
        // check if data block is unallocated
        if(data_block_number != 0)
        {
            data_block_numbers[i] = 0;
            allocated_data_block_number = data_block_number;
            break;
        }
    }

    ssize_t temp = altfs_superblock->s_freelist_head;
    if(allocated_data_block_number == 0)
    {
        allocated_data_block_number = altfs_superblock->s_freelist_head;
        altfs_superblock->s_freelist_head = data_block_numbers[0];
        data_block_numbers[0] = 0;
        if(!altfs_write_superblock())
        {
            return -1;
        }
    }

    if(!altfs_write_block(temp, buffer)) {
        fuse_log(FUSE_LOG_ERR, "%s Error writing free list block\n", ALLOCATE_DATA_BLOCK);
        return -1;
    }

    return allocated_data_block_number;
}

char* read_data_block(ssize_t index)
{
    if(index <= INODE_BLOCK_COUNT || index > BLOCK_COUNT)
    {
        fuse_log(FUSE_LOG_ERR, "%s Invalid block index for read: %ld\n", READ_DATA_BLOCK, index);
        return NULL;
    }

    char* buffer = (char*)malloc(BLOCK_SIZE);
    if(altfs_read_block(index, buffer))
    {
        return buffer;
    }
    return NULL;
}

bool write_data_block(ssize_t index, char* buffer)
{
    if(index <= INODE_BLOCK_COUNT || index > BLOCK_COUNT)
    {
        fuse_log(FUSE_LOG_ERR, "%s Invalid block index for write: %ld\n", WRITE_DATA_BLOCK, index);
        return false;
    }
    return altfs_write_block(index, buffer);
}

bool free_data_block(ssize_t index) {
    if(index <= INODE_BLOCK_COUNT || index > BLOCK_COUNT)
    {
        fuse_log(FUSE_LOG_ERR, "%s Invalid block index to free: %ld\n", FREE_DATA_BLOCK, index);
        return false;
    }

    char buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);
    altfs_write_block(index, buffer);

    // fuse_log(FUSE_LOG_DEBUG, "%s Superblock freelist head: %ld ; datablock being freed: %ld.\n", FREE_DATA_BLOCK, altfs_superblock->s_freelist_head, index);

    // If all blocks were full, make the block being free'd the freelist head.
    if(altfs_superblock->s_freelist_head == 0)
    {
        altfs_superblock->s_freelist_head = index;
        if(!altfs_write_superblock())
        {
            fuse_log(FUSE_LOG_ERR, "%s Error in writing superblock after freelist head update.\n", FREE_DATA_BLOCK);
            return false;
        }
        fuse_log(FUSE_LOG_DEBUG, "%s Superblock freelist head was 0, updated to %ld.\n", FREE_DATA_BLOCK, altfs_superblock->s_freelist_head);
    }

    if(!altfs_read_block(altfs_superblock->s_freelist_head, buffer))
    {
        fuse_log(FUSE_LOG_ERR, "%s Error in reading superblock block at freelist head.\n", FREE_DATA_BLOCK);
        return false;
    }

    ssize_t* data_block_numbers = (ssize_t*)buffer;
    ssize_t added = 0;
    for(ssize_t i = 1; i < NUM_OF_ADDRESSES_PER_BLOCK; ++i)
    {
        if(data_block_numbers[i] == 0)
        {
            data_block_numbers[i] = index;
            added = 1;
            break;
        }
    }

    if(added == 0)
    {
        ssize_t temp = altfs_superblock->s_freelist_head;
        altfs_superblock->s_freelist_head = index;

        if(!altfs_write_superblock()) {
            fuse_log(FUSE_LOG_ERR, "%s Error in writing superblock after freelist head update (2).\n", FREE_DATA_BLOCK);
            return false;
        }
        memset(buffer, 0, BLOCK_SIZE);
        memcpy(buffer, &temp, ADDRESS_SIZE);
        if(!altfs_write_block(index, buffer))
        {
            fuse_log(FUSE_LOG_ERR, "%s Error in writing next free block number to the block that was freed.\n", FREE_DATA_BLOCK);
            return false;
        }
    } else
    {
        if(!altfs_write_block(altfs_superblock->s_freelist_head, buffer)) {
            fuse_log(FUSE_LOG_ERR, "%s Error in writing number of the block freed to the freelist head.\n", FREE_DATA_BLOCK);
            return false;
        }
    }
    return true;
}
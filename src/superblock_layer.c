#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<stdio.h>
#include<fuse.h>
#include "../header/superblock_layer.h"
#include "../header/disk_layer.h"

static struct superblock* altfs_superblock = NULL;

// Initializes superblock and writes to block 0
bool altfs_create_superblock()
{
    altfs_superblock = (struct superblock*)malloc(sizeof(struct superblock));
    // fd 0,1,2 = input, output, error => first ino will start from 3
    altfs_superblock->s_first_ino = 3;
    //altfs_superblock->s_inode_size = sizeof(struct inode);
    altfs_superblock->s_num_of_inodes_per_block = (BLOCK_SIZE) / sizeof(struct inode);
    altfs_superblock->s_inodes_count = INODE_BLOCK_COUNT*altfs_superblock->s_num_of_inodes_per_block;
    // first data block will be the head of free list
    altfs_superblock->s_freelist_head = INODE_BLOCK_COUNT+1;

    char buff[BLOCK_SIZE];
    memset(buff, 0, BLOCK_SIZE);
    memcpy(buff, altfs_superblock, sizeof(struct superblock));
    fuse_log(FUSE_LOG_DEBUG, "%s Writing superblock...\n",ALTFS_CREATE_SUPERBLOCK);
    if (!altfs_write_block(0, buff))
    {
        fuse_log(FUSE_LOG_ERR, "%s Error writing to superblock\n", ALTFS_CREATE_SUPERBLOCK);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Finished writing superblock...\n",ALTFS_CREATE_SUPERBLOCK);
    return true;
}

// Creates ilist by initializing inode struct for all inodes
// Does a write operation block wise for the number of inodes contained in a block
bool altfs_create_ilist()
{
    // TODO: Verify all elements of struct are getting initialized correctly
    // TODO: The ilist size is set to number of inodes - Check that this works fine
    //struct inode inodeObj[sizeof(struct inode)];
    struct inode inodeObj[altfs_superblock->s_inodes_count];
    // TODO: Adding loop for initialization of inodes - Verify this
    for(ssize_t k = 0; k < altfs_superblock->s_inodes_count; k++)
    {
        for(ssize_t i = 0; i < NUM_OF_DIRECT_BLOCKS; i++)
            inodeObj[k].i_direct_blocks[i] = 0;
        inodeObj[k].i_allocated = false;
        inodeObj[k].i_single_indirect = 0;
        inodeObj[k].i_double_indirect = 0;
        inodeObj[k].i_triple_indirect = 0;
        inodeObj[k].i_links_count = 0;
        inodeObj[k].i_file_size = 0;
        inodeObj[k].i_blocks_num = 0;
    }
    // initialize ilist for all blocks meant for inodes
    // start with index = 1 since superblock will take block 0
    char buffer[BLOCK_SIZE];
    fuse_log(FUSE_LOG_DEBUG, "%s Creating ilist...\n", ALTFS_CREATE_ILIST);
    for(ssize_t blocknum = 1; blocknum <= INODE_BLOCK_COUNT; blocknum++)
    {
        ssize_t offset = 0;
        memset(buffer, 0, BLOCK_SIZE);
        for(ssize_t inodenum = 0; inodenum < altfs_superblock->s_num_of_inodes_per_block; inodenum++)
        {
            memcpy(buffer + offset, inodeObj, sizeof(struct inode));
            offset += sizeof(struct inode);
        }
        fuse_log(FUSE_LOG_DEBUG, "%s Writing blocknum %ld and buffer %s\n",ALTFS_CREATE_ILIST, blocknum, *buffer);
        if (!altfs_write_block(blocknum, buffer)){
            fuse_log(FUSE_LOG_ERR, "%s Error writing to inode block number %d\n", ALTFS_CREATE_ILIST, blocknum);
            return false;
        }
    }
    return true;
}


bool altfs_create_freelist()
{
    // iterate over all blocks meant for free list 
    // Initialize the addresses
    ssize_t blocknum = altfs_superblock->s_freelist_head;
    char buffer[BLOCK_SIZE];
    ssize_t nullvalue = 0;

    for(ssize_t i = 0; i < NUM_OF_FREE_LIST_BLOCKS; i++)
    {
        ssize_t currblocknum = blocknum;
        // initialize block with zeroes
        memset(buffer, 0, BLOCK_SIZE);
        ssize_t offset = 0;
        // for each block, initialize addresses
        // Starts from index 1 since first one witll have block address off next free list block
        for(ssize_t j=1; j < NUM_OF_ADDRESSES_PER_BLOCK; j++)
        {
            offset += ADDRESS_SIZE; 
            blocknum += 1;
            // start writing at buffer + offset since first index will have address of next free block
            if (blocknum >= BLOCK_COUNT)
                memcpy(buffer+offset, &nullvalue, ADDRESS_SIZE);
            else
                memcpy(buffer+offset, &nullvalue, ADDRESS_SIZE);
        }
        blocknum += 1;
        if (blocknum >= BLOCK_COUNT)
            memcpy(buffer, &nullvalue, ADDRESS_SIZE);
        else
            memcpy(buffer, &blocknum, ADDRESS_SIZE);
        
        if (!altfs_write_block(currblocknum, buffer))  
        {
            fuse_log(FUSE_LOG_ERR, "%s Error writing block number %d to free list\n", ALTFS_CREATE_FREELIST, currblocknum);
            return false;
        }
        // TODO: Check on below check
        //if (blocknum >= BLOCK_COUNT)
        //    break;
    }
    return true;
}

bool altfs_makefs()
{
    // TODO: Check if this is remounting 
    // if yes, read superblock and skip init steps
    bool allocateFSMemory = altfs_alloc_memory();
    if (!allocateFSMemory)
    {
        fuse_log(FUSE_LOG_ERR, "%s Error allocating memory while initializing FS\n",ALTFS_MAKEFS);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Allocated memory for FS while initializing\n", ALTFS_MAKEFS);

    bool createSuperBlock = altfs_create_superblock();
    if (!createSuperBlock)
    {
        fuse_log(FUSE_LOG_ERR, "%s Error creating superblock while initializing FS\n",ALTFS_MAKEFS);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Successfully created superblock for FS\n", ALTFS_MAKEFS);

    bool createInodeList = altfs_create_ilist();
    if (!createInodeList)
    {
        fuse_log(FUSE_LOG_ERR, "%s Error creating inode list while initializing FS\n",ALTFS_MAKEFS);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Successfully created inode list for FS\n", ALTFS_MAKEFS);

    bool createFreeList = altfs_create_freelist();
    if (!createFreeList)
    {
        fuse_log(FUSE_LOG_ERR, "%s Error creating free list while initializing FS\n",ALTFS_MAKEFS);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Successfully created free list for FS\n", ALTFS_MAKEFS);
    return true;
}
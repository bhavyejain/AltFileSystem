#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<stdio.h>
#include<fuse.h>
#include "../header/data_structure_layer.h";
#include "../header/disk_layer.h";

static struct superblock* altfs_superblock = NULL;

bool altfs_create_superblock()
{
    altfs_superblock = (struct superblock*)malloc(sizeof(struct superblock));
    // fd 0,1,2 = input, output, error => first ino will start from 3
    altfs_superblock->s_first_ino = 3;
    altfs_superblock->s_inode_size = sizeof(struct inode);
    altfs_superblock->s_inodes_count = 
    altfs_superblock->s_freelist_head = 

    char buff[BLOCK_SIZE];
    memset(buff, 0, BLOCK_SIZE);
    memcpy(buff, altfs_superblock, sizeof(struct superblock));
    if (!altfs_write_block)
    {
        fuse_log(FUSE_LOG_ERR, "%s Error writing to superblock\n", ALTFS_CREATE_SUPERBLOCK);
        return false;
    }
    return true;
}

bool altfs_create_ilist()
{
    // TODO: Verify all elements of struct are getting initialized correctly
    struct inode inodeObj[sizeof(struct inode)] = {0};
    inodeObj->i_allocated = false;
    // initialize ilist for all blocks meant for inodes
    // start with index = 1 since superblock will take block 0
    char buffer[BLOCK_SIZE];
    for(int blocknum = 1; blocknum <= INODE_BLOCK_COUNT; blocknum++)
    {
        int offset = 0;
        memset(buff, 0, BLOCK_SIZE);
        for(int inodenum = 0; inodenum < altfs_superblock->)
    }

    return true;
}

bool altfs_create_freelist()
{
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
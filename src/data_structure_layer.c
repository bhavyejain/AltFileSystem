#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<stdio.h>
#include<fuse.h>
#include "../header/data_structure_layer.h";
#include "../header/disk_layer.h";

bool altfs_create_superblock()
{
    struct superblock* altfs_superblock = (struct superblock*)malloc(sizeof(struct superblock));
    // TODO: Is 3 correct?
    altfs_superblock->s_first_ino = 3;
    altfs_superblock->s_inode_size = sizeof(struct inode);
    altfs_superblock->s_inodes_count = 
    altfs_superblock->s_freelist_head = 
    return true;
}

bool altfs_create_ilist()
{
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
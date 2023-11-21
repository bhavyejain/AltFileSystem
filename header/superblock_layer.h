#ifndef __SUPERBLOCK_LAYER__
#define __SUPERBLOCK_LAYER__

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
 #include <sys/types.h>
#include "disk_layer.h"

/*
 * This file creates the inode, superblock, freelist data structures
 * and performs operations required for these data structures along
 * with reading and writing from and to data blocks
*/

#define ALTFS_MAKEFS "altfs_makefs"
#define ALTFS_CREATE_SUPERBLOCK "altfs_create_superblock"
#define ALTFS_CREATE_ILIST "altfs_create_ilist"
#define ALTFS_CREATE_FREELIST "altfs_create_freelist"

#define NUM_OF_DIRECT_BLOCKS 12;
#define ADDRESS_SIZE 8;
// TODO: check if all inodes have max size files, can disk handle the scenario
#define INODE_BLOCK_COUNT (BLOCK_COUNT/10) // 10% blocks reserved for inodes TODO: Check if we can reduce this
#define NUM_OF_DATA_BLOCKS (BLOCK_COUNT - INODE_BLOCK_COUNT - 1) // -1 for superblock
#define NUM_OF_ADDRESSES_PER_BLOCK (BLOCK_SIZE / ADDRESS_SIZE) // Assuming each address is 8B 
#define NUM_OF_FREE_LIST_BLOCKS (NUM_OF_DATA_BLOCKS / NUM_OF_ADDRESSES_PER_BLOCK + 1) // Num of free list blocks = data required to store that many addresses


// Data structure for inode
// Follows a structure similar to ext4 - https://www.kernel.org/doc/html/latest/filesystems/ext4/inodes.html?highlight=inode
struct inode 
{
    mode_t i_mode; // permission mode
    ssize_t i_uid; // lower 16-bits of owner id
    ssize_t i_gid; // lower 16-bits of group id
    //ssize_t i_size_lo;
    time_t i_atime; // last access time
    time_t i_ctime; // last inode change time
    time_t i_mtime; // last data modification time
    time_t i_dtime; // deletion time
    ssize_t i_links_count; // hard link count
    ssize_t i_file_size; // file size
    ssize_t i_blocks_num; // num of blocks the file has
    bool i_allocated; // flag to indicate if inode is allocated
    // TODO: Kept number of direct blocks as 12 in sync with ext4
    ssize_t i_direct_blocks[NUM_OF_DIRECT_BLOCKS];
    ssize_t i_single_indirect;
    ssize_t i_double_indirect;
    ssize_t i_triple_indirect;
};

struct superblock
{
    ssize_t s_inodes_count;
    //ssize_t s_free_blocks_count;
    ssize_t s_first_ino; // first non-reserved inode
    ssize_t s_freelist_head;
    ssize_t s_inode_size; // ??? check if required or not
    ssize_t s_num_of_inodes_per_block;
};

// makefs - Calls disk layer to allocate memory, initializes superblock, i-list and free list
bool altfs_makefs();

ssize_t altfs_create_inode();

//struct inode* altfs_read_inode(ssize_t )

#endif
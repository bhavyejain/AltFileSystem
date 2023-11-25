#ifndef __INITIALIZE_FS_OPS__
#define __INITIALIZE_FS_OPS__

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include <sys/types.h>
#include "disk_layer.h"
#include "superblock_layer.h"
#include "data_block_ops.h"

#define INITIALIZE_FS "initialize_fs"
#define ADD_DIRECTORY_ENTRY "add_directory_entry"
#define GET_DBLOCK_FROM_IBLOCK "get_disk_block_from_inode_block"

#define ROOT_INODE_NUM ((ssize_t) 2)
#define DEFAULT_PERMISSIONS (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#define NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR ((ssize_t) (BLOCK_SIZE / ADDRESS_SIZE)) // 4096/8 = 512
#define NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR ((ssize_t) (NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR * NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)) // 512*512
#define NUM_OF_TRIPLE_INDIRECT_BLOCK_ADDR ((ssize_t) (NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR * NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)) // 512*512*512
#define CACHE_CAPACITY ((ssize_t) 100000) // TODO: Check if increasing this improves performance
#define DIRECT_PLUS_SINGLE_INDIRECT_ADDR ((ssize_t) (NUM_OF_DIRECT_BLOCKS + NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR))
#define DIRECT_PLUS_SINGLE_DOUBLE_INDIRECT_ADDR ((ssize_t) (NUM_OF_DIRECT_BLOCKS + NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR + NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR))

// Directory entry contants
#define RECORD_LENGTH ((unsigned short) 2) // use unsigned short
#define RECORD_ALLOCATED ((ssize_t) 1)  // use bool or char
#define RECORD_INUM ((ssize_t) 8)   // TODO: Make sure search file, unlink (file layer), and altfs_readdir (fuse layer) use this and not INODE_SIZE while reading directory records
#define RECORD_FIXED_LEN ((unsigned short)(RECORD_LENGTH + RECORD_ALLOCATED + RECORD_INUM))
#define MAX_FILE_NAME_LENGTH ((ssize_t) 255)
#define LAST_POSSIBLE_RECORD ((ssize_t)(BLOCK_SIZE - RECORD_FIXED_LEN))

/*
Get the physical disk block number for a given file and logical block number in the file.

@param node: Constant pointer to the file's inode
@param logical_block_num: Logical block number in the file
@param prev_indirect_block: Holds the number of the data block that contains the last level of indirect addresses for the block number (logical_block_num - 1). Provide 0 for every out-of-order access.

@return The physical data block number.
*/
ssize_t get_disk_block_from_inode_block(const struct inode* const file_inode, ssize_t file_block_num, ssize_t* prev_indirect_block);

#endif
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
#define ALTFS_ADD_INODE_ENTRY "altfs_add_inode_entry"
#define GET_DBLOCK_FROM_FBLOCK "get_data_block_from_file_block"

#define ROOT_INODE_NUM ((ssize_t) 2)
#define FILE_NAME_SIZE ((ssize_t) 2)
#define ADDRESS_PTR_SIZE ((ssize_t) 8)
#define INODE_SIZE ((ssize_t) 8)
#define DEFAULT_PERMISSIONS (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#define MAX_FILE_NAME_LENGTH ((ssize_t) 255)
#define NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR ((ssize_t) (BLOCK_SIZE / ADDRESS_SIZE)) // 4096/8 = 512
#define NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR ((ssize_t) (NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR*(BLOCK_SIZE / ADDRESS_SIZE))) // 512*512
#define NUM_OF_TRIPLE_INDIRECT_BLOCK_ADDR ((ssize_t) (NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR*(BLOCK_SIZE / ADDRESS_SIZE))) // 512*512*512
#define CACHE_CAPACITY ((ssize_t) 100000) // TODO: Check if increasing this improves performance

// Get data block number for given file block
ssize_t get_data_block_from_file_block(const struct inode* const file_inode, ssize_t file_block_num)

#endif
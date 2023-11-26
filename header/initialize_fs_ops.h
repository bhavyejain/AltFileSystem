#ifndef __INITIALIZE_FS_OPS__
#define __INITIALIZE_FS_OPS__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "data_block_ops.h"
#include "disk_layer.h"
#include "superblock_layer.h"

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
#define OFFSET_TO_INUM ((ssize_t) RECORD_LENGTH + RECORD_ALLOCATED)

/*
Get the physical disk block number for a given file and logical block number in the file.

@param node: Constant pointer to the file's inode
@param logical_block_num: Logical block number in the file
@param prev_indirect_block: Holds the number of the data block that contains the last level of indirect addresses for the block number (logical_block_num - 1). Provide 0 for every out-of-order access.

@return The physical data block number.
*/
ssize_t get_disk_block_from_inode_block(const struct inode* const file_inode, ssize_t file_block_num, ssize_t* prev_indirect_block);

/*
A directory entry (record) in altfs looks like:
| Total entry length (2) | Allocated (1) | INUM (8) | Name (variable len) |

The allocated byte tells us if the entry is re-usable to store another file name.
If the value is 111 (true), then an active file holds this entry.
If the value is 000 (false), then it is reusable.

@param dir_inode: Pointer to the directory (parent) inode.
@param child_inum: Inode number of the file being added as an entry.
@param file_name: Name of the file being added.

@return true or false
*/
bool add_directory_entry(struct inode* dir_inode, ssize_t child_inum, char* file_name);

/*
Initializes the file system.

@return True if success, false if failure.
*/
bool initialize_fs();

/*
Get inum for given file path

@param file_path: File path whose inode number is required

@return ssize_t Inode number corresponding to the given file path
*/
ssize_t name_i(const char* const file_path);

#endif
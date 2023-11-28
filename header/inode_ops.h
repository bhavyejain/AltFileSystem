#ifndef __INODE_OPS__
#define __INODE_OPS__

#include <sys/types.h>

#include "common_includes.h"
#include "superblock_layer.h"

#define ALLOCATE_INODE "allocate_inode"
#define GET_INODE "get_inode"
#define WRITE_INODE "write_inode"
#define FREE_INODE "free_inode"
#define GET_DBLOCK_FROM_IBLOCK "get_disk_block_from_inode_block"

/*
Allocates a new inode.

@return Inode number or -1.
*/
ssize_t allocate_inode();

/*
Gets the inode corresponding to the given number.

@param inum: The inode number.

@return Pointer to struct inode or NULL
*/
struct inode* get_inode(ssize_t inum);

/*
Write the inode to the given number.

@param inum: The number to be associated with the inode.
@param node: The inode to be written.

@return True or false.
*/
bool write_inode(ssize_t inum, struct inode* node);

/*
Free the inode AND associated data blocks.

@param inum: The inode number.

@return True or false
*/
bool free_inode(ssize_t inum);

/*
Helper to check if an inode number is valid.
*/
bool is_valid_inode_number(ssize_t inum);

/*
Get the physical disk block number for a given file and logical block number in the file.

@param node: Constant pointer to the file's inode
@param logical_block_num: Logical block number in the file
@param prev_indirect_block: Holds the number of the data block that contains the last level of indirect addresses for the block number (logical_block_num - 1). Provide 0 for every out-of-order access.

@return The physical data block number.
*/
ssize_t get_disk_block_from_inode_block(const struct inode* const file_inode, ssize_t file_block_num, ssize_t* prev_indirect_block);

#endif
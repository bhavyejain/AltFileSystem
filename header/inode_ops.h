#ifndef __INODE_OPS__
#define __INODE_OPS__

#include <sys/types.h>

#include "common_includes.h"
#include "superblock_layer.h"

#define ALLOCATE_INODE "allocate_inode"
#define FREE_INODE "free_inode"
#define GET_DBLOCK_FROM_IBLOCK "get_disk_block_from_inode_block"
#define GET_INODE "get_inode"
#define WRITE_INODE "write_inode"

#define ROOT_INODE_NUM ((ssize_t) 2)
#define DEFAULT_PERMISSIONS (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#define NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR ((ssize_t) (BLOCK_SIZE / ADDRESS_SIZE)) // 4096/8 = 512
#define NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR ((ssize_t) (NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR * NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)) // 512*512
#define NUM_OF_TRIPLE_INDIRECT_BLOCK_ADDR ((ssize_t) (NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR * NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)) // 512*512*512
#define CACHE_CAPACITY ((ssize_t) 100000) // TODO: Check if increasing this improves performance
#define DIRECT_PLUS_SINGLE_INDIRECT_ADDR ((ssize_t) (NUM_OF_DIRECT_BLOCKS + NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR))
#define DIRECT_PLUS_SINGLE_DOUBLE_INDIRECT_ADDR ((ssize_t) (NUM_OF_DIRECT_BLOCKS + NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR + NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR))

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

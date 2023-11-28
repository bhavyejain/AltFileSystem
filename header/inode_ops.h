#ifndef __INODE_OPS__
#define __INODE_OPS__

#include <sys/types.h>

#include "common_includes.h"
#include "superblock_layer.h"

#define ALLOCATE_INODE "allocate_inode"
#define GET_INODE "get_inode"
#define WRITE_INODE "write_inode"
#define FREE_INODE "free_inode"

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

#endif
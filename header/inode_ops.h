#ifndef __INODE_OPS__
#define __INODE_OPS__

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include <sys/types.h>
#include "disk_layer.h"
#include "superblock_layer.h"
#include "data_block_ops.h"

#define ALLOCATE_INODE "allocate_inode"
#define GET_INODE "get_inode"
#define WRITE_INODE "write_inode"
#define FREE_INODE "free_inode"

/*
@func: Allocates a new inode.

@return: inode number or -1.
*/
ssize_t allocate_inode();

/*
@func: Gets the inode corresponding to the given number.

@param inum: The inode number.

@return: inode or NULL
*/
struct inode* get_inode(ssize_t inum);

/*
@func: Write the inode to the given number.

@param inum: The number to be associated with the inode.
@param node: The inode to be written.

@return: true or false.
*/
bool write_inode(ssize_t inum, struct inode* node);

/*
@func: Free the inode AND associated data blocks.

@param inum: The inode number.

@return: true or false
*/
bool free_inode(ssize_t inum);

/*
@func: Helper to check if an inode number is valid.
*/
bool is_valid_inode_number(ssize_t inum);

#endif
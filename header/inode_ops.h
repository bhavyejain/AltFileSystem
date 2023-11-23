#ifndef __INODE_OPS__
#define __INODE_OPS__

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include <sys/types.h>
#include "disk_layer.h"
#include "superblock_layer.h"
#include "data_block_ops.h"

/*
@func: Allocates a new inode.

@return: inode number(index) or -1.
*/
ssize_t allocate_inode();

/*
@func: Gets the inode at given index.

@param index: The inode number.

@return: inode or NULL
*/
struct inode* get_inode(ssize_t index);

/*
@func: Read the inode referred to by inodenum.

@param inodenum: The number to be associated with the inode.

@return: true or false.
*/
bool read_inode(ssize_t inodenum);

/*
@func: Write the inode to the index.

@param index: The number to be associated with the inode.
@param node: The inode to be written.

@return: true or false.
*/
bool write_inode(ssize_t index, struct inode* node);

/*
@func: Free the inode AND associated data blocks.

@param index: The inode number.

@return: true or false
*/
bool free_inode(ssize_t index);

#endif
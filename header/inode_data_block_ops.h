#ifndef __INODE_DATA_BLOCKOPS__
#define __INODE_DATA_BLOCKOPS__

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include <sys/types.h>
#include "disk_layer.h"
#include "superblock_layer.h"
#include "data_block_ops.h"

#define ADD_DATABLOCK_TO_INODE "add_datablock_to_inode"
#define OVERWRITE_DATABLOCK_TO_INODE "overwrite_datablock_to_inode"
#define REMOVE_DATABLOCK_FROM_INODE "remove_datablocks_from_inode"

/*
@func: Adds a new data block to an existing inode

@param inodeObj: The pointer to the inode to which the data block needs to be added

@param data_block_num: Data block number of the block to be added to the inode

@return: bool: true if operation is successful
*/

bool add_datablock_to_inode(struct inode* inodeObj, const ssize_t data_block_num);

/*
@func: Overwrites an existing data block in an inode 

@param inodeObj: The pointer to the inode to which the data block needs to be overwritten

@param data_block_num: File block number of the block to be overwritten from 

@param data_block_num: Data block number of the block to be overwritten in the inode

@return: bool: true if operation is successful
*/

bool overwrite_datablock_to_inode(struct inode* inodeObj, ssize_t file_block_num, ssize_t data_block_num);

/*
@func: Remove a data block to an existing inode

@param inodeObj: The pointer to the inode to which the data block needs to be added

@param file_block_num: File block number of the block to be removed from the inode

@return: bool: true if operation is successful
*/

bool remove_datablocks_from_inode(struct inode* inodeObj, const ssize_t file_block_num);

#endif
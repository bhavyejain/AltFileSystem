#ifndef __DATA_BLOCK_OPS__
#define __DATA_BLOCK_OPS__

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include <sys/types.h>
#include "disk_layer.h"
#include "superblock_layer.h"
#include "disk_layer.h"

#define ALLOCATE_DATA_BLOCK "allocate_data_block"
#define READ_DATA_BLOCK "read_data_block"
#define WRITE_DATA_BLOCK "write_data_block"
#define FREE_DATA_BLOCK "free_data_block"

/*
@func: Allocate a new data block

@returns: dblocknum on success; -1 on failure
*/
ssize_t allocate_data_block();

/*
@func: Read information contained in a data block.

@param index: The data block number.

@return: Buffer with contents or NULL
*/
char* read_data_block(ssize_t index);

/*
@func: write information to a data block

@param index: The data block number.
@param buf: the buffer which contains data to write

@return: success or failure
*/
bool write_data_block(ssize_t index, char* buffer);

/*
@func: Free a data block.

@param index: The data block number.

@return: success or failure
*/
bool free_data_block(ssize_t index);

#endif
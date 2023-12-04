#ifndef __DATA_BLOCK_OPS__
#define __DATA_BLOCK_OPS__

#include <sys/types.h>

#include "common_includes.h"

#define ALLOCATE_DATA_BLOCK "allocate_data_block"
#define FREE_DATA_BLOCK "free_data_block"
#define READ_DATA_BLOCK "read_data_block"
#define WRITE_DATA_BLOCK "write_data_block"

/*
Allocate a new data block

@return Data block number on success or -1 on failure
*/
ssize_t allocate_data_block();

/*
Read information contained in a data block.

@param index: The data block number.

@return Buffer with contents or NULL
*/
char* read_data_block(ssize_t index);

/*
Write information to a data block

@param index: The data block number.
@param buf: the buffer which contains data to write

@return Success or failure
*/
bool write_data_block(ssize_t index, char* buffer);

/*
Free a data block.

@param index: The data block number.

@return Success or failure
*/
bool free_data_block(ssize_t index);

#endif

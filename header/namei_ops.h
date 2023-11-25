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

// Struct used to store information regarding a file's position in the directory
struct fileposition {
    char *p_block;
    ssize_t p_plock_num;
    ssize_t l_block_num;
    ssize_t start_pos;
    ssize_t prev_entry;
};

/*
Return position of file in dir

@param file_name: File name to search.

@param parent_inode: Parent inode 

@return struct file_pos denoting the position of the file in directory
*/
struct fileposition get_file_position_in_dir(const char* const file_name, const struct inode* const parent_inode);

/*
Checks if dir given by inum is empty

@param inum: The inode number.

@return True if dir is empty
*/
bool is_dir_empty();

/*
Copy the parent path to the given buffer given the path and path length

@param buffer: the buffer to store parent path.

@param path: file path

@param path_len: length of file path

@return True if the operation was successful
*/
bool copy_parent_path(char* const buffer, const char* const path, ssize_t path_len);

/*
Copy the child fiel name to the given buffer given the path and path length

@param buffer: the buffer to store path.

@param path: file path

@param path_len: length of file path

@return True if the operation was successful
*/
bool copy_child_file_name(char* const buffer, const char* const path, ssize_t path_len);

/*
Get inum for given file path

@param file_path: File path whose inode number is required

@return ssize_t Inode number corresponding to the given file path
*/

ssize_t name_i(const char* const file_path);

#endif
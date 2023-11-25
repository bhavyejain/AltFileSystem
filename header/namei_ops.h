#ifndef __NAMEI_OPS__
#define __NAMEI_OPS__

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include <sys/types.h>
#include "disk_layer.h"
#include "superblock_layer.h"
#include "data_block_ops.h"
#include "inode_ops.h"
#include "initialize_fs_ops.h"

#define GET_FILE_POS_IN_DIR "get_file_position_in_dir"
#define IS_DIR_EMPTY "is_dir_empty"
#define COPY_PARENT_PATH "copy_parent_path"
#define COPY_CHILD_FILE_NAME "copy_child_file_name"
#define NAME_I "name_i"

/*
Struct used to store the position of a file's inode inside it's parent directory
*/
struct fileposition {
    char *p_block; // contents of physical data block
    ssize_t p_plock_num; // physical data block number
    ssize_t offset; // offset in the dir's physical data block where the file's inode is stored
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
#ifndef __PATH_HELPERS__
#define __PATH_HELPERS__

#include <sys/types.h>
#include <sys/stat.h>

#include "common_includes.h"
#include "superblock_layer.h"

#define GET_FILE_POS_IN_DIR "get_file_position_in_dir"
#define IS_DIR_EMPTY "is_dir_empty"
#define COPY_PARENT_PATH "copy_parent_path"
#define COPY_CHILD_FILE_NAME "copy_child_file_name"

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
Return index of last char of parent path from given path

@param path: File path

@param path_length: Length of file path

@return ssize_t index of last char of parent path
*/

ssize_t get_last_index_of_parent_path(const char* const path, ssize_t path_length);

/*
Copy the parent path to the given buffer given the path and path length

@param buffer: the buffer to store parent path.

@param path: file path

@param path_len: length of file path

@return True if the operation was successful
*/
bool copy_parent_path(char* const buffer, const char* const path, ssize_t path_len);

/*
Copy the file name to the given buffer given the entire path and path length

@param buffer: the buffer to store path.

@param path: file path

@param path_len: length of file path

@return True if the operation was successful
*/
bool copy_file_name(char* const buffer, const char* const path, ssize_t path_len);

#endif
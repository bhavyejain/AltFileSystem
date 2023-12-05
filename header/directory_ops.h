#ifndef __DIRECTORY_OPS__
#define __DIRECTORY_OPS__

#include <sys/stat.h>
#include <sys/types.h>

#include "common_includes.h"
#include "superblock_layer.h"

#define ADD_DIRECTORY_ENTRY "add_directory_entry"
#define COPY_CHILD_FILE_NAME "copy_child_file_name"
#define COPY_PARENT_PATH "copy_parent_path"
#define GET_FILE_POS_IN_DIR "get_file_position_in_dir"
#define IS_DIR_EMPTY "is_dir_empty"
#define NAME_I "name_i"
#define SETUP_FILESYSTEM "setup_filesystem"

// Directory entry contants
#define MAX_FILE_NAME_LENGTH ((ssize_t) 255)
#define RECORD_LENGTH ((unsigned short) 2) // use unsigned short
#define RECORD_INUM ((ssize_t) 8)   // TODO: Make sure search file, unlink (file layer), and altfs_readdir (fuse layer) use this and not INODE_SIZE while reading directory records
#define RECORD_FIXED_LEN ((unsigned short)(RECORD_LENGTH + RECORD_INUM))
#define LAST_POSSIBLE_RECORD ((ssize_t)(BLOCK_SIZE - RECORD_FIXED_LEN))

/*
Struct used to store the position of a file's inode inside it's parent directory
*/
struct fileposition {
    char *p_block; // contents of physical data block
    ssize_t p_plock_num; // physical data block number
    ssize_t offset; // offset in the dir's physical data block where the file's inode is stored
};

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

/*
A directory entry (record) in altfs looks like:
[ Total entry length (2) | INUM (8) | Name (variable len) ]

Total entry length is 2 + 8 + length of name in bytes.
There are no holes in a single data block, so a total entry length of 0 means there are no records from that point on.
INUM is the inode number of the file being pointed to.
Name is the file name.

@param dir_inode: Double pointer to the directory (parent) inode.
@param child_inum: Inode number of the file being added as an entry.
@param file_name: Name of the file being added.

@return true or false
*/
bool add_directory_entry(struct inode** dir_inode, ssize_t child_inum, char* file_name);

/*
Remove record for a file from a directory's records.

@param dir_inode: Double pointer to the directory (parent) inode.
@param file_name: Name of the file being removed.

@return true or false
*/
bool remove_directory_entry(struct inode** dir_inode, char* file_name);

/*
Return position of file in dir

@param file_name: File name to search.

@param parent_inode: Parent inode 

@return struct file_pos denoting the position of the file in directory
*/
struct fileposition get_file_position_in_dir(const char* const file_name, const struct inode* const parent_inode);

/*
Sets up the file system.

@return True if success, false if failure.
*/
bool setup_filesystem();

/*
Get inum for given file path

@param file_path: File path whose inode number is required

@return ssize_t Inode number corresponding to the given file path, or -1.
*/
ssize_t name_i(const char* const file_path);

/*
Helper to remove a path's entry from the inode cache.

@param path: Full path of the entry to be removed.

@return True is success, false if failure.
*/
bool remove_from_inode_cache(const char* path);

/*
Remove all entries from the inode cache.
*/
void flush_inode_cache(bool create);

#endif

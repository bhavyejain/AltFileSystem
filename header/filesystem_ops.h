#ifndef __FILESYSTEM_OPS__
#define __FILESYSTEM_OPS__

#include <sys/stat.h>
#include <sys/types.h>

#include "common_includes.h"
#include "superblock_layer.h"

#define INITIALIZE_FS "initialize_fs"
#define ADD_DIRECTORY_ENTRY "add_directory_entry"
#define NAME_I "name_i"

// Directory entry contants
#define RECORD_LENGTH ((unsigned short) 2) // use unsigned short
#define RECORD_INUM ((ssize_t) 8)   // TODO: Make sure search file, unlink (file layer), and altfs_readdir (fuse layer) use this and not INODE_SIZE while reading directory records
#define RECORD_FIXED_LEN ((unsigned short)(RECORD_LENGTH + RECORD_INUM))
#define MAX_FILE_NAME_LENGTH ((ssize_t) 255)
#define LAST_POSSIBLE_RECORD ((ssize_t)(BLOCK_SIZE - RECORD_FIXED_LEN))

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
Helper to remove a path's entry from the inode cache.

@param path: Full path of the entry to be removed.

@return True is success, false if failure.
*/
bool remove_from_inode_cache(char* path);

/*
Initializes the file system.

@return True if success, false if failure.
*/
bool initialize_fs();

/*
Get inum for given file path

@param file_path: File path whose inode number is required

@return ssize_t Inode number corresponding to the given file path
*/
ssize_t name_i(const char* const file_path);

#endif
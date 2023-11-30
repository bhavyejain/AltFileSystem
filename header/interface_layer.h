#ifndef __INTERFACE_LAYER__
#define __INTERFACE_LAYER__

#include <sys/types.h>

#include "common_includes.h"

#define MKDIR "altfs_mkdir"
#define MKNOD "altfs_mknod"
#define TRUNCATE "altfs_truncate"
#define UNLINK "altfs_unlink"
#define CLOSE "altfs_close"
#define OPEN "altfs_open"
#define READ "altfs_read"
#define WRITE "altfs_write"
#define ACCESS "altfs_access"
#define CHMOD "altfs_chmod"
#define GETATTR "altfs_getattr"
#define READDIR "altfs_readdir"
#define RENAME "altfs_rename"

// Wrapper over setup_filesystem()
bool altfs_init();

/*
Get attributes for the file at path.

@param path: A c-string that contains the full path.
@param st: Double pointer to the stat object to be filled.

@return 0 if successful, -errno otherwise.
*/
ssize_t altfs_getattr(const char* path, struct stat** st);

/*
Check if path is accessible and has permissions.

@param path: A c-string that contains the full path.

@return 0 if can access, -errno otherwise.
*/
ssize_t altfs_access(const char* path);

/*
Creates a new directory in the file system.

@param path: A c-string that contains the full path.
@param mode: Permission bits for the directory.

@return bool: success or failure
*/
bool altfs_mkdir(const char* path, mode_t mode);

/*
Read contents of a directory.

@param path: A c-string that contains the full path.
@param buff: The buffer to fill with the info.
@param filler: Helper function to fill the buffer with data.

@return 0 if successful, -errno otherwise.
*/
ssize_t altfs_readdir(const char* path, void* buff, fuse_fill_dir_t filler);

/*
Create a special file.

@param path: A c-string that contains the full path.
@param mode: Specifies the major or minor device numbers.
@param dev: If mode indicates a block or special device, dev is
    a specification of a character or block I/O device and the
    superblock of the device.

@return bool: success or failure
*/
bool altfs_mknod(const char* path, mode_t mode, dev_t dev);

/*
Removes the link between the entity at path and its parent. If link count becomes 0, remove the file.

@param path: A c-string that contains the full path.

@return 0 if success, -errornum if failure.
*/
ssize_t altfs_unlink(const char* path);

/*
Open a file.

@param path: A c-string that contains the full path.
@param oflag: O_FLAGS for the open call.

@return Inode number if success, -errornum if failure.
*/
ssize_t altfs_open(const char* path, ssize_t oflag);

ssize_t altfs_close(ssize_t file_descriptor);

/*
Read bytes from a file.

@param path: A c-string that contains the full path.
@param buff: The output buffer that needs to be populated with the read data.
@param n_bytes: The number of bytes that are to be read.
@param offset: The byte-offset in the file from where the data is supposed to be read.

@return The actual number of bytes read.
*/
ssize_t altfs_read(const char* path, void* buff, size_t nbytes, off_t offset);

/*
Write bytes to a file.

@param path: A c-string that contains the full path.
@param buff: The bytes that need to be written to the file.
@param n_bytes: The number of bytes that are to be written.
@param offset: The byte-offset in the file from where the data is supposed to be written.

@return Actual number of bytes written if data written, -1 if not written.
*/
ssize_t altfs_write(const char* path, const char* buff, size_t nbytes, off_t offset);

/*
Truncate a file after the given offset.

@param path: A c-string that contains the full path.
@param offset: The byte-offset in the file from where the file is supposed to be truncated.

@return 0 if success, -1 if failure.
*/
ssize_t altfs_truncate(const char* path, size_t offset);

/*
Change the permission bits of the inode corresponding to the path.

@param path: A c-string that contains the full path.
@param mode: Permission bits for the file.

@return 0 if success, -errno otherwise.
*/
ssize_t altfs_chmod(const char* path, mode_t mode);

/*
Rename a file.

@param from: A c-string that contains the full existing path.
@param to: A c-string that contains the full final path.

@return 0 if successful, -errno otherwise.
*/
ssize_t altfs_rename(const char *from, const char *to);

#endif
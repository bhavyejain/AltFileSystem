#ifndef __INTERFACE_LAYER__
#define __INTERFACE_LAYER__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

/*
Creates a new directory in the file system.

@param path: A c-string that contains the full path.
@param mode: Permission bits for the directory.

@return bool: success or failure
*/
bool altfs_mkdir(const char* path, mode_t mode);

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

ssize_t altfs_truncate(const char* path, size_t offset);

ssize_t altfs_unlink(const char* path);

ssize_t altfs_close(ssize_t file_descriptor);

ssize_t altfs_open(const char* path, ssize_t oflag);

/*
Read bytes from a file.

@param path: A c-string that contains the full path.
@param buff: The output buffer that needs to be populated with the read data.
@param n_bytes: The number of bytes that are to be read.
@param offset: The byte-offset in the file from where the data is supposed to be read.

@return The actual number of bytes read.
*/
ssize_t altfs_read(const char* path, void* buff, size_t nbytes, size_t offset);

/*
Write bytes to a file.

@param path: A c-string that contains the full path.
@param buff: The bytes that need to be written to the file.
@param n_bytes: The number of bytes that are to be written.
@param offset: The byte-offset in the file from where the data is supposed to be written.

@return n_bytes if data written, -1 if not written.
*/
ssize_t altfs_write(const char* path, void* buff, size_t nbytes, size_t offset);

#endif
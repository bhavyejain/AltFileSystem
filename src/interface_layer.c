#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <fuse.h>

#include "../header/interface_layer.h"
#include "../header/superblock_layer.h"

ssize_t create_new_file(const char* const path, struct inode** buff, mode_t mode)
{}

bool altfs_mkdir(const char* path, mode_t mode)
{}

bool altfs_mknod(const char* path, mode_t mode, dev_t dev)
{}

ssize_t altfs_truncate(const char* path, size_t offset)
{}

ssize_t altfs_unlink(const char* path)
{}

ssize_t altfs_close(ssize_t file_descriptor)
{}

ssize_t altfs_open(const char* path, ssize_t oflag)
{}

ssize_t altfs_read(const char* path, void* buff, size_t nbytes, size_t offset)
{}

ssize_t altfs_write(const char* path, void* buff, size_t nbytes, size_t offset)
{}
#ifndef __FUSE_LAYER_H__
#define __FUSE_LAYER_H__

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 31
#endif

#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stddef.h>

#include "interface_layer.h"

static const struct fuse_operations fuse_ops;

// TODO 
// opendir
// symlink

static int fuse_access(const char* path, int mode);

static int fuse_chown(const char* path, uid_t uid, gid_t gid);

static int fuse_chmod(const char* path, mode_t mode);

static int fuse_create(const char* path, mode_t mode, struct fuse_file_info* fi);

static int fuse_getattr(const char* path, struct stat* st);

static int fuse_open(const char* path, struct fuse_file_info* fi);

static int fuse_read(const char* path, char* buff, size_t size, off_t offset, struct fuse_file_info* fi);

static int fuse_readdir(const char* path, void* buff, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);

static int fuse_rmdir(const char* path);

static int fuse_mkdir(const char* path, mode_t mode);

static int fuse_mknod(const char* path, mode_t mode, dev_t dev);

static int fuse_utimens(const char* path, const struct timespec time_spec[2]);

static int fuse_truncate(const char* path, off_t offset);

static int fuse_unlink(const char* path);

static int fuse_write(const char* path, const char* buff, size_t size, off_t offset, struct fuse_file_info* fi);

static int fuse_rename(const char *from, const char *to);

#endif
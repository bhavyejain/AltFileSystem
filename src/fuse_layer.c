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
#include <stdbool.h>

#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"
#include "../src/inode_ops.c"
#include "../src/data_block_ops.c"
#include "../src/inode_data_block_ops.c"
#include "../src/inode_cache.c"
#include "../src/directory_ops.c"
#include "../src/interface_layer.c"

static int my_access(const char* path, int mode)
{
    return altfs_access(path);
}

static int my_chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info *fi)
{
    // TODO: Check for improvement
    return 0; // out of scope as of now as only one user
}

static int my_chmod(const char* path, mode_t mode, struct fuse_file_info *fi)
{
    return altfs_chmod(path, mode);
} 

static int my_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
    bool status = false;
    if(fi->flags & O_CREAT)
    {
        status = altfs_mknod(path, S_IFREG|mode, -1);
    }
    else
    {
        status = altfs_mknod(path, S_IFREG|0775, -1);
    }

    if(!status)
    {
        return -1;
    }
    return 0;
}

static int my_getattr(const char* path, struct stat* st, struct fuse_file_info *fi)
{
    return altfs_getattr(path, &st);
}

static int my_open(const char* path, struct fuse_file_info* fi)
{
    ssize_t inum = altfs_open(path, fi->flags);
    if(inum <= -1)
    {
        return inum;
    }
    return 0;
}

static int my_read(const char* path, char* buff, size_t size, off_t offset, struct fuse_file_info* fi)
{
    ssize_t nbytes = altfs_read(path, buff, size, offset);
    return nbytes;
}

static int my_readdir(const char* path, void* buff, fuse_fill_dir_t filler, off_t offset,
                        struct fuse_file_info* fi, enum fuse_readdir_flags)
{
    (void) offset;
    (void) fi;
    return altfs_readdir(path, buff, filler);
}

static int my_rmdir(const char* path)
{
    return altfs_unlink(path);
}

static int my_mkdir(const char* path, mode_t mode)
{
    bool status = altfs_mkdir(path, mode);
    if(!status)
    {
        return -1;
    }
    return 0;
}

static int my_mknod(const char* path, mode_t mode, dev_t dev)
{
    bool status = altfs_mknod(path, mode, dev);
    if(!status)
    {
        return -1;
    }
    return 0;
}

static int my_utimens(const char* path, const struct timespec time_spec[2], struct fuse_file_info *fi)
{
    // TODO: check
    return 0;
}

static int my_truncate(const char* path, off_t offset, struct fuse_file_info *fi)
{
    return altfs_truncate(path, offset);
}

static int my_unlink(const char* path)
{
    return altfs_unlink(path);
}

static int my_write(const char* path, const char* buff, size_t size, off_t offset, struct fuse_file_info* fi)
{
    return altfs_write(path, buff, size, offset);
}

static int my_rename(const char *from, const char *to, unsigned int flags)
{
    return altfs_rename(from, to);
}

static void my_destroy(void *private_data)
{
    altfs_destroy();
}

static const struct fuse_operations my_ops = {
    .access   = my_access,
    .chown    = my_chown,
    .chmod    = my_chmod,
    .create   = my_create,
    .getattr  = my_getattr,
    .mkdir    = my_mkdir,
    .rmdir    = my_rmdir,
    .unlink   = my_unlink,
    .truncate = my_truncate,
    .mknod    = my_mknod,
    .readdir  = my_readdir,
    .open     = my_open,
    .read     = my_read,
    .write    = my_write,
    .utimens  = my_utimens,
    .rename   = my_rename,
    .destroy = my_destroy,
};

int main(int argc, char* argv[])
{
    if(!altfs_init())
    {
        printf("AltFS initialization failed!\n");
        return 0;
    }
    umask(0000);
    return fuse_main(argc, argv, &my_ops, NULL);
}

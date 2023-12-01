#include <errno.h>
#include <stdbool.h>

#include "../header/fuse_layer.h"

static const struct fuse_operations fuse_ops = {
    .access   = fuse_access,
    .chown    = fuse_chown,
    .chmod    = fuse_chmod,
    .create   = fuse_create,
    .getattr  = fuse_getattr,
    .mkdir    = fuse_mkdir,
    .rmdir    = fuse_rmdir,
    .unlink   = fuse_unlink,
    .truncate = fuse_truncate,
    .mknod    = fuse_mknod,
    .readdir  = fuse_readdir,
    .open     = fuse_open,
    .read     = fuse_read,
    .write    = fuse_write,
    .utimens  = fuse_utimens,
    .rename   = fuse_rename,
    .destroy = fuse_destroy,
};

static int fuse_access(const char* path, int mode)
{
    return altfs_access(path);
}

static int fuse_chown(const char* path, uid_t uid, gid_t gid)
{
    // TODO: Check for improvement
    return 0; // out of scope as of now as only one user
}

static int fuse_chmod(const char* path, mode_t mode)
{
    return altfs_chmod(path, mode);
} 

static int fuse_create(const char* path, mode_t mode, struct fuse_file_info* fi)
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

static int fuse_getattr(const char* path, struct stat* st)
{
    return altfs_getattr(path, &st);
}

static int fuse_open(const char* path, struct fuse_file_info* fi)
{
    ssize_t inum = altfs_open(path, fi->flags);
    if(inum <= -1)
    {
        return -1;
    }
    return 0;
}

static int fuse_read(const char* path, char* buff, size_t size, off_t offset, struct fuse_file_info* fi)
{
    ssize_t nbytes = altfs_read(path, buff, size, offset);
    return nbytes;
}

static int fuse_readdir(const char* path, void* buff, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info* fi){
    (void) offset;
    (void) fi;
    return altfs_readdir(path, buff, filler);
}

static int fuse_rmdir(const char* path)
{
    return altfs_unlink(path);
}

static int fuse_mkdir(const char* path, mode_t mode)
{
    bool status = altfs_mkdir(path, mode);
    if(!status)
    {
        return -1;
    }
    return 0;
}

static int fuse_mknod(const char* path, mode_t mode, dev_t dev)
{
    bool status = altfs_mknod(path, mode, dev);
    if(!status)
    {
        return -1;
    }
    return 0;
}

static int fuse_utimens(const char* path, const struct timespec time_spec[2])
{
    // TODO: check
    return 0;
}

static int fuse_truncate(const char* path, off_t offset)
{
    return altfs_truncate(path, offset);
}

static int fuse_unlink(const char* path)
{
    return altfs_unlink(path);
}

static int fuse_write(const char* path, const char* buff, size_t size, off_t offset, struct fuse_file_info* fi)
{
    return altfs_write(path, buff, size, offset);
}

static int fuse_rename(const char *from, const char *to, unsigned int flags)
{
    return altfs_rename(from, to);
}

static void fuse_destroy(void *private_data)
{
    altfs_destroy();
}

int main(int argc, char* argv[]){
    if(!altfs_init())
    {
        printf("AltFS initialization failed!\n");
        return 0;
    }
    umask(0000);
    return fuse_main(argc, argv, &fuse_ops, NULL);
}

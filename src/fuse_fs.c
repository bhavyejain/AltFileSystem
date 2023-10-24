#define FUSE_USE_VERSION 31

#define _XOPEN_SOURCE 500

#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

static char rootdir[100];

static void get_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, rootdir);
    strncat(fpath, path, PATH_MAX);
}

static void *altfs_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	(void) conn;
	cfg->kernel_cache = 1;
	return NULL;
}

static int altfs_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
	int res;
	// char fpath[PATH_MAX];
    // get_fullpath(fpath, path);

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int altfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
	int retstat = 0;
    DIR *dp;
    struct dirent *de;
    
    dp = (DIR *) (uintptr_t) fi->fh;

    de = readdir(dp);
    if (de == 0)
        return -errno;

	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);

    do {
        if (filler(buf, de->d_name, NULL, 0, 0) != 0)
            return -ENOMEM;
    } while ((de = readdir(dp)) != NULL);
    
    return retstat;

	// DIR *dp;
	// struct dirent *de;

	// (void) offset;
	// (void) fi;
	// (void) flags;

	// dp = opendir(fpath);
	// if (dp == NULL)
	// 	return -errno;

	// while ((de = readdir(dp)) != NULL) {
	// 	struct stat st;
	// 	memset(&st, 0, sizeof(st));
	// 	st.st_ino = de->d_ino;
	// 	st.st_mode = de->d_type << 12;
	// 	if (filler(buf, de->d_name, &st, 0, 0))
	// 		break;
	// }

	// closedir(dp);
	// return 0;
}

static int altfs_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	// char fpath[PATH_MAX];
    // get_fullpath(fpath, path);

	res = open(path, fi->flags);
	if (res < 0)
		return -errno;

	fi->fh = res;

	return res;
}

static int altfs_truncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	int res;
	// char fpath[PATH_MAX];
    // get_fullpath(fpath, path);

	if (fi != NULL)
		res = ftruncate(fi->fh, size);
	else
		res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int altfs_write(const char* path, const char *buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	int fd;
	int res;

	// char fpath[PATH_MAX];
	// get_fullpath(fpath, path);
	fd = open(path, O_CREAT | O_WRONLY, 0644);
	
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

static int altfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	int fd;
	int res;
	// char fpath[PATH_MAX];
    // get_fullpath(fpath, path);

	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

// The suffix altfs stands for AltFileSystem
// static const struct fuse_operations altfs_oper = {
// 	.init       = altfs_init,
// 	.getattr	= altfs_getattr,
// 	.readdir	= altfs_readdir,
// 	.open		= altfs_open,
// 	.read		= altfs_read,
// 	.truncate   = altfs_truncate,
//  .write      = altfs_write,
// };

static const struct fuse_operations altfs_oper = {
	.init       = altfs_init,
	.getattr	= altfs_getattr,
	.readdir	= altfs_readdir,
	.open		= altfs_open,
	.read		= altfs_read,
	.truncate   = altfs_truncate,
    .write      = altfs_write,
};

int main(int argc, char *argv[])
{
	fprintf(stderr, "In main!!\n");
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	char *temp = realpath(argv[argc-1], NULL);
	// char *temp = get_current_dir_name();
	strcpy(rootdir, temp);
	fprintf(stderr, "rootdir: %s\n", rootdir);

	int i = creat("~/hello.txt", 0666);
	ret = fuse_main(argc, argv, &altfs_oper, NULL);

	fuse_opt_free_args(&args);
	return ret;
}
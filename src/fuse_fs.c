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

static char *fname = "/myfile.txt";

static void get_file_path(char fpath[PATH_MAX], const char *path)
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
	fprintf(stderr, "Inside getattr\n");
	fprintf(stderr, "Path: %s\n", path);
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else {
		char fpath[PATH_MAX];
		get_file_path(fpath, fname);
		fprintf(stderr, "FPath: %s\n", fpath);
		res = lstat(fpath, stbuf);
	}

	if (res == -1)
		return -errno;

	return 0;
}

static int altfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
				off_t offset, struct fuse_file_info *fi,
				enum fuse_readdir_flags flags)
{
	fprintf(stderr, "Inside readdir\n");
	fprintf(stderr, "Path: %s\n", path);
	int retstat = 0;
	DIR *dp;
	struct dirent *de;

	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);

	dp = (DIR *) (uintptr_t) fi->fh;

	if (dp != NULL) {
		fprintf(stderr, "attempting to read dir\n");
		de = readdir(dp);
		fprintf(stderr, "got first readdir\n");
		if (de == 0)
		{
			fprintf(stderr, "error in reading dir");
			return -errno;
		}

		do {
			fprintf(stderr, "adding entry %s", de->d_name);
			if (filler(buf, de->d_name, NULL, 0, 0) != 0) {
				fprintf(stderr, "buffer full");
				return -ENOMEM;
			}
		} while ((de = readdir(dp)) != NULL);
	} else {
		fprintf(stderr, "dp is null\n");
	}

	return retstat;
}

static int altfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	fprintf(stderr, "Inside create\n");
	fprintf(stderr, "Path: %s\n", path);
	int res;

	char fpath[PATH_MAX];
	get_file_path(fpath, fname);
	fprintf(stderr, "FPath: %s\n", fpath);

	res = creat(fpath, mode);
	fprintf(stderr, "Result of create: %d\n", res);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

static int altfs_open(const char *path, struct fuse_file_info *fi)
{
	fprintf(stderr, "Inside open\n");
	fprintf(stderr, "Path: %s\n", path);
	int res;

	char fpath[PATH_MAX];
	get_file_path(fpath, fname);
	fprintf(stderr, "FPath: %s\n", fpath);

	fprintf(stderr, "attempting to open\n");
	res = open(fpath, fi->flags);
	fprintf(stderr, "Result: %d\n", res);
	if (res < 0) {
		fprintf(stderr, "error in opening!\n");
		return -errno;
	}

	fi->fh = res;

	return 0;
}

static int altfs_truncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	fprintf(stderr, "Inside truncate\n");
	fprintf(stderr, "Path: %s\n", path);
	int res;

	char fpath[PATH_MAX];
	get_file_path(fpath, fname);
	fprintf(stderr, "FPath: %s\n", fpath);

	if (fi != NULL)
		res = ftruncate(fi->fh, size);
	else
		res = truncate(fpath, size);

	if (res == -1)
		return -errno;

	return 0;
}

static int altfs_write(const char* path, const char *buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
	fprintf(stderr, "Inside write\n");
	fprintf(stderr, "Path: %s\n", path);
	int fd;
	int res;

	char fpath[PATH_MAX];
	get_file_path(fpath, fname);
	fprintf(stderr, "FPath: %s\n", fpath);

	fd = open(fpath, O_CREAT | O_WRONLY, 0644);

	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);

	FILE *output;
	// TODO: Check if 4096 is enough and handles all kinds of special strings
	char buf2[4096];
	snprintf(buf2, sizeof(buf2), "echo -e \"Mail from fuse :) \\n\\n %s \\n\" | sendmail swathi_bhat@ucsb.edu", buf);
	output = popen(buf2, "r");
	fprintf(stderr, "Executing command: %s\n", buf2);
	
	if (output == NULL)
		fprintf(stderr, "Failed to exec command\n");
	
	res = pclose(output);
	if (res == -1)
		res = -errno;

	return res;
}

static int altfs_read(const char *path, char *buf, size_t size, off_t offset,
struct fuse_file_info *fi)
{
	fprintf(stderr, "Inside read\n");
	fprintf(stderr, "Path: %s\n", path);
	int fd;
	int res;

	char fpath[PATH_MAX];
	get_file_path(fpath, fname);
	fprintf(stderr, "FPath: %s\n", fpath);

	fd = open(fpath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static const struct fuse_operations altfs_oper = {
	.init       = altfs_init,
	.getattr	= altfs_getattr,
	.readdir	= altfs_readdir,
	.create		= altfs_create,
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

	// char *temp = realpath(argv[argc-1], NULL);
	char *temp = get_current_dir_name();
	strcpy(rootdir, temp);
	fprintf(stderr, "rootdir: %s\n", rootdir);

	ret = fuse_main(argc, argv, &altfs_oper, NULL);

	fuse_opt_free_args(&args);
	return ret;
}
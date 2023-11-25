#include <errno.h>
#include <fuse.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "../header/initialize_fs_ops.h"
#include "../header/inode_ops.h"
#include "../header/interface_layer.h"
#include "../header/namei_ops.h"
#include "../header/superblock_layer.h"

#define CREATE_NEW_FILE "create_new_file"

ssize_t create_new_file(const char* const path, struct inode** buff, mode_t mode)
{
    // getting parent path
    ssize_t path_len = strlen(path);
    char parent_path[path_len+1];
    if(!copy_parent_path(parent_path, path, path_len)){
        fuse_log(FUSE_LOG_ERR, "%s : No parent path exists for path: %s.\n", CREATE_NEW_FILE, path);
        return -ENOENT;
    }

    ssize_t parent_inode_num = name_i(parent_path);
    if(parent_inode_num==-1){
        fuse_log(FUSE_LOG_ERR, "%s : Invalid parent path: %s.\n", CREATE_NEW_FILE, parent_path);
        return -ENOENT;
    }

    ssize_t child_inode_num = name_i(path);
    if(child_inode_num != -1){
        fuse_log(FUSE_LOG_ERR, "%s : File already exists: %s.\n", CREATE_NEW_FILE, path);
        return -EEXIST;
    }

    struct inode* parent_inode = get_inode(parent_inode_num);
    // Check if parent is a directory
    if(!S_ISDIR(parent_inode->i_mode)){
        fuse_log(FUSE_LOG_ERR, "%s : Parent is not a directory: %s.\n", CREATE_NEW_FILE, parent_path);
        free_memory(parent_inode);
        return -ENOENT;
    }
    // Check parent write permission
    if(!(bool)(parent_inode->i_mode & S_IWUSR))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Parent directory does not have write permission: %s.\n", CREATE_NEW_FILE, parent_path);
        free_memory(parent_inode);
        return -EACCES;
    }

    char child_name[path_len+1];
    if(!copy_child_file_name(child_name, path, path_len)){
        fuse_log(FUSE_LOG_ERR, "%s : Error getting child file name from path: %s.\n", CREATE_NEW_FILE, path);
        return -1;
    }

    // Allocate new inode and add directory entry
    child_inode_num = allocate_inode();
    if(child_inode_num == -1){
        fuse_log(FUSE_LOG_ERR, "%s : Could not allocate an inode for file.\n", CREATE_NEW_FILE);
        free_memory(parent_inode);
        return -EDQUOT;
    }
    if(!add_directory_entry(parent_inode, child_inode_num, child_name)){
        fuse_log(FUSE_LOG_ERR, "%s : Could not add directory entry for file.\n", CREATE_NEW_FILE);
        free_inode(child_inode_num);
        free_memory(parent_inode);
        return -EDQUOT;
    }

    time_t curr_time = time(NULL);
    parent_inode->i_mtime = curr_time;
    parent_inode->i_status_change_time = curr_time;
    // TODO: Check when is access time updated
    parent_inode->i_atime = curr_time;
    if(!write_inode(parent_inode_num, parent_inode)){
        fuse_log(FUSE_LOG_ERR, "%s : Could not write directory inode.\n", CREATE_NEW_FILE);
        free_memory(parent_inode);
        return -1;
    }

    *buff = get_inode(child_inode_num);
    (*buff)->i_links_count++;
    (*buff)->i_mode = mode;
    (*buff)->i_blocks_num = 0;
    (*buff)->i_file_size = 0;
    (*buff)->i_atime = curr_time;
    (*buff)->i_mtime = curr_time;
    (*buff)->i_ctime = curr_time;
    (*buff)->i_status_change_time = curr_time;
    if(!write_inode(child_inode_num, *buff))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Could not write child file inode.\n", CREATE_NEW_FILE);
        free_memory(*buff);
        return -1;
    }

    free_memory(parent_inode);
    return child_inode_num;
}

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
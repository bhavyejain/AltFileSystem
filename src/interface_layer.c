#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "../header/data_block_ops.h"
#include "../header/disk_layer.h"
#include "../header/initialize_fs_ops.h"
#include "../header/inode_data_block_ops.h"
#include "../header/inode_ops.h"
#include "../header/interface_layer.h"
#include "../header/namei_ops.h"
#include "../header/superblock_layer.h"

#define CREATE_NEW_FILE "create_new_file"

/*
Allocates a new inode for a file and fills the inode with default values.
*/
ssize_t create_new_file(const char* const path, struct inode** buff, mode_t mode, ssize_t* parent_inum)
{
    // getting parent path 
    // TODO: check if this works correctly when paths are of the forms below:
    // ./a/b/c.txt, ./.././../a/b/c.txt, /a.txt
    ssize_t path_len = strlen(path);
    char parent_path[path_len+1];
    if(!copy_parent_path(parent_path, path, path_len))
    {
        fuse_log(FUSE_LOG_ERR, "%s : No parent path exists for path: %s.\n", CREATE_NEW_FILE, path);
        return -ENOENT;
    }

    ssize_t parent_inode_num = name_i(parent_path);
    if(parent_inode_num == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Invalid parent path: %s.\n", CREATE_NEW_FILE, parent_path);
        return -ENOENT;
    }

    ssize_t child_inode_num = name_i(path);
    if(child_inode_num != -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : File already exists: %s.\n", CREATE_NEW_FILE, path);
        return -EEXIST;
    }

    struct inode* parent_inode = get_inode(parent_inode_num);
    // Check if parent is a directory
    if(!S_ISDIR(parent_inode->i_mode))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Parent is not a directory: %s.\n", CREATE_NEW_FILE, parent_path);
        altfs_free_memory(parent_inode);
        return -ENOENT;
    }
    // Check parent write permission
    if(!(bool)(parent_inode->i_mode & S_IWUSR))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Parent directory does not have write permission: %s.\n", CREATE_NEW_FILE, parent_path);
        altfs_free_memory(parent_inode);
        return -EACCES;
    }

    char child_name[path_len+1];
    if(!copy_file_name(child_name, path, path_len))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Error getting child file name from path: %s.\n", CREATE_NEW_FILE, path);
        return -1;
    }

    // Allocate new inode and add directory entry
    child_inode_num = allocate_inode();
    if(child_inode_num == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Could not allocate an inode for file.\n", CREATE_NEW_FILE);
        altfs_free_memory(parent_inode);
        return -EDQUOT;
    }
    if(!add_directory_entry(&parent_inode, child_inode_num, child_name))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Could not add directory entry for file.\n", CREATE_NEW_FILE);
        free_inode(child_inode_num);
        altfs_free_memory(parent_inode);
        return -EDQUOT;
    }

    time_t curr_time = time(NULL);
    parent_inode->i_mtime = curr_time;
    parent_inode->i_ctime = curr_time;
    if(!write_inode(parent_inode_num, parent_inode))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Could not write directory inode.\n", CREATE_NEW_FILE);
        altfs_free_memory(parent_inode);
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
    (*buff)->i_child_num = 0;

    if(!write_inode(child_inode_num, *buff))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Could not write child file inode.\n", CREATE_NEW_FILE);
        altfs_free_memory(*buff);
        return -1;
    }

    altfs_free_memory(parent_inode);
    *parent_inum = parent_inode_num;
    return child_inode_num;
}

bool is_empty_dir(struct inode** dir_inode){
    if((*dir_inode)->i_child_num > 2)
    {
        fuse_log(FUSE_LOG_DEBUG, "is_empty_dir : Directory has more than 2 entries: %ld.\n", (*dir_inode)->i_child_num);
        return false;
    }
    return true;
}

bool altfs_mkdir(const char* path, mode_t mode)
{
    struct inode* dir_inode = NULL;
    ssize_t parent_inum;
    ssize_t dir_inode_num = create_new_file(path, &dir_inode, S_IFDIR|mode, &parent_inum);
    if(dir_inode_num <= -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to allot inode with error: %ld.\n", MKDIR, dir_inode_num);
        altfs_free_memory(dir_inode);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s : Alloted inode number %ld to directory.\n", MKDIR, dir_inode_num);

    char* name = ".";
    if(!add_directory_entry(&dir_inode, dir_inode_num, name))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to add directory entry: %s.\n", MKDIR, name);
        altfs_free_memory(dir_inode);
        return false;
    }

    name = "..";
    if(!add_directory_entry(&dir_inode, parent_inum, name))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to add directory entry: %s.\n", MKDIR, name);
        altfs_free_memory(dir_inode);
        return false;
    }

    if(!write_inode(dir_inode_num, dir_inode))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to write inode: %ld.\n", MKDIR, dir_inode_num);
        altfs_free_memory(dir_inode);
        return false;
    }
    altfs_free_memory(dir_inode);
    return true;
}

bool altfs_mknod(const char* path, mode_t mode, dev_t dev)
{
    // TODO: Buggy code
    dev = 0; // Silence error until used
    struct inode* node = NULL;

    fuse_log(FUSE_LOG_DEBUG, "%s : MKNOD mode passed: %ld.\n", MKNOD, mode);

    ssize_t inum = create_new_file(path, &node, mode);

    if (inum <= -1) {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to allot inode with error: %ld.\n", MKNOD, inum);
        altfs_free_memory(node);
        return false;
    }
    if(!write_inode(inum, node)){
        fuse_log(FUSE_LOG_ERR, "%s : Failed to write inode: %ld.\n", MKNOD, inum);
        altfs_free_memory(node);
        return false;
    }
    altfs_free_memory(node);
    return true;
}

ssize_t altfs_truncate(const char* path, size_t offset)
{
    ssize_t inum = name_i(path);
    if(inum == -1){
        fuse_log(FUSE_LOG_ERR, "%s : Failed to get inode number for path: %s.\n", TRUNCATE, path);
        return -1;
    }

    struct inode* node = get_inode(inum);
    if(offset > node->i_file_size){
        fuse_log(FUSE_LOG_ERR, "%s : Failed to truncate: offset is greater than file size.\n", TRUNCATE);
        altfs_free_memory(node);
        return -1;
    }
    if(offset == 0 && node->i_file_size == 0){
        fuse_log(FUSE_LOG_ERR, "%s : Failed to truncate: Offset is 0 and file size is also 0.\n", TRUNCATE);
        altfs_free_memory(node);
        return 0;
    }

    ssize_t i_block_num = offset / BLOCK_SIZE;
    if(node->i_blocks_num > i_block_num + 1){
        // Remove everything from i_block_num + 1
        remove_datablocks_from_inode(node, i_block_num + 1);
    }

    // Get data block for offset
    ssize_t prev = 0;
    ssize_t d_block_num = get_disk_block_from_inode_block(node, i_block_num, &prev);
    if(d_block_num <= 0){
        fuse_log(FUSE_LOG_ERR, "%s : Failed to get data block number from inode block number.\n", TRUNCATE);
        return -1;
    }
    char* data_block = read_data_block(d_block_num);

    ssize_t block_offset = offset % BLOCK_SIZE;
    memset(data_block + block_offset, 0, BLOCK_SIZE - block_offset);
    write_dblock(d_block_num, data_block);
    altfs_free_memory(data_block);

    node->file_size = offset;
    write_inode(inum, node);
    altfs_free_memory(node);
    return 0;
}

ssize_t altfs_unlink(const char* path)
{
    ssize_t path_len = strlen(path);
    char child_name[path_len + 1];
    if(!copy_file_name(child_name, path, path_len))
    {
        fuse_log(FUSE_LOG_ERR, "%s : No child found for path: %s.\n", UNLINK, path);
        return -1;
    }

    char parent_path[path_len + 1];
    if(!copy_parent_path(parent_path, path, path_len))
    {
        fuse_log(FUSE_LOG_ERR, "%s : No parent path exists for path: %s.\n", UNLINK, path);
        return -EINVAL;
    }

    ssize_t inum = name_i(path);
    if(inum == -1){
        fuse_log(FUSE_LOG_ERR, "%s : Failed to get inode number for path: %s.\n", UNLINK, path);
        return -1;
    }
    struct inode* node = get_inode(inum);
    // If path is a directory which is not empty, fail operation
    if(S_ISDIR(node->i_mode) && is_empty_dir(node))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to unnlink, dir is not empty: %s.\n", UNLINK, path);
        altfs_free_memory(node);
        return -ENOTEMPTY;
    }

    ssize_t parent_inum = name_i(parent_path);
    if(parent_path == -1){
        fuse_log(FUSE_LOG_ERR, "%s : Failed to get inode number for parent path: %s.\n", UNLINK, parent_path);
        return -1;
    }
    struct inode* parent = get_inode(parent_inum);

    struct fileposition file_pos = get_file_position_in_dir(child_name, parent);
    if(file_pos.offset == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : No entry found for child %s in parent %s.\n", UNLINK, child_name, parent_path);
        altfs_free_memory(node);
        altfs_free_memory(parent);
        return -1;
    }

    fuse_log(FUSE_LOG_DEBUG, "%s : Record data block contents before removal:\n%s\n" UNLINK, file_pos.p_block);
    // Rewrite block with child entry deleted
    char buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);
    memcpy(buffer, file_pos.p_block, file_pos.offset);
    unsigned short rec_len = ((unsigned short*)(file_pos.p_block + file_pos.offset))[0];
    ssize_t next_offset = file_pos.offset + rec_len;
    if(next_offset < BLOCK_SIZE)
    {
        memcpy(buffer + file_pos.offset, file_pos.p_block + next_offset, BLOCK_SIZE - next_offset);
    }
    altfs_write_block(file_pos.p_plock_num, buffer);
    fuse_log(FUSE_LOG_DEBUG, "%s : Record data block contents after removal:\n%s\n" UNLINK, buffer);
    altfs_free_memory(file_pos.p_block);

    time_t curr_time = time(NULL);
    parent->i_ctime = curr_time;
    parent->i_mtime = curr_time;

    node->i_links_count--;
    if(node->i_links_count == 0)
    {
        remove_from_inode_cache(path);
        free_inode(inum);
    } else
    {
        node->i_status_change_time = curr_time;
        write_inode(inum, node);
    }
    write_inode(parent_inum, parent);
    altfs_free_memory(parent);
    altfs_free_memory(node);
    return 0;
}

// Check if this even needs to be implemented.
ssize_t altfs_close(ssize_t file_descriptor)
{
    return 0;
}

ssize_t altfs_open(const char* path, ssize_t oflag)
{
    ssize_t inum = name_i(path);
    bool created = false;

    // Create new file if required
    if(inum <= -1 && (oflag & O_CREAT))
    {
        struct inode* node = NULL;
        ssize_t parent_inum;
        inum = create_new_file(path, &node, S_IFREG|DEFAULT_PERMISSIONS, &parent_inum);
        if(inum <= -1)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Error creating file %s, errno: %ld.\n", OPEN, path, inum);
            return -1;
        }
        write_inode(inum, node);
        altfs_free_memory(node);
        created = true;
    }
    else if(inum <= -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : File %s not found.\n", OPEN, path);
        return -ENOENT;
    }

    // Permissions check
    struct inode* node = get_inode(inum);
    if((oflag & O_RDONLY) || (oflag & O_RDWR)) // Needs read permission
    {
        if(!(bool)(node->i_mode & S_IRUSR))
        {
            fuse_log(FUSE_LOG_ERR, "%s : File %s does not have read permission.\n", OPEN, path);
            return -EACCES;
        }
    }
    if((oflag & O_WRONLY) || (oflag & O_RDWR)) // Needs read permission
    {
        if(!(bool)(node->i_mode & S_IWUSR))
        {
            fuse_log(FUSE_LOG_ERR, "%s : File %s does not have write permission.\n", OPEN, path);
            return -EACCES;
        }
    }

    // Truncate if required
    if(oflag & O_TRUNC)
    {
        if(altfs_truncate(path, 0) == -1)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Error truncating file %s.\n", OPEN, path);
            return -1;
        }
        // If existing file, update times.
        if(!created)
        {
            time_t curr_time = time(NULL);
            node->i_ctime = curr_time;
            node->i_mtime = curr_time;
            write_inode(inum, node);
        }
    }
    altfs_free_memory(node);
    return inum;
}

ssize_t altfs_read(const char* path, void* buff, size_t nbytes, size_t offset)
{
    if(nbytes == 0)
    {
        return 0;
    }
    if(offset < 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Negative offset provided: %ld.\n", READ, offset);
        return -EINVAL;
    }
    memset(buff, 0, nbytes);

    ssize_t inum = name_i(path);
    if (inum == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Inode for file %s not found.\n", READ, path);
        return -ENOENT;
    }

    struct inode* node= get_inode(inum);
    if (node->i_file_size == 0)
    {
        return 0;
    }
    if (offset > node->file_size)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Offset %ld is greater than file size %ld.\n", READ, offset, node->i_file_size);
        return -EOVERFLOW;
    }
    // Update nbytes when read_len from offset exceeds file_size. 
    if (offset + nbytes > node->i_file_size)
    {
        nbytes = node->i_file_size - offset;
    }

    ssize_t start_i_block = offset / BLOCK_SIZE; // First logical block to read from
    ssize_t start_block_offset = offset % BLOCK_SIZE; // The starting offset in first block from where to read
    ssize_t end_i_block = (offset + nbytes) / BLOCK_SIZE; // Last logical block to read from
    ssize_t end_block_offset = BLOCK_SIZE - (offset + nbytes) % BLOCK_SIZE; // Ending offet in last block till where to read

    ssize_t blocks_to_read = end_i_block - start_i_block + 1; // Number of blocks that need to be read
    fuse_log(FUSE_LOG_DEBUG, "%s : Number of blocks to read from: %ld.\n", READ, blocks_to_read);

    size_t bytes_read = 0;
    ssize_t dblock_num;
    char* buf_read = NULL;

    ssize_t prev_block = 0;
    if(blocks_to_read == 1)
    {
        // Only 1 block to be read
        dblock_num = get_disk_block_from_inode_block(node, start_i_block, &prev_block);
        if(dblock_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Could not get disk block number for inode block %ld.\n", READ, start_i_block);
            altfs_free_memory(node);
            return -1;
        }

        buf_read = read_data_block(dblock_num);
        if(buf_read == NULL)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Could not read block for data block number %ld.\n", READ, dblock_num);
            altfs_free_memory(node);
            return -1;
        }
        memcpy(buff, buf_read + start_block_offset, nbytes);
        bytes_read = nbytes;
    }
    else
    {
        //when there are multiple blocks to read
        for(ssize_t i = 0; i < blocks_to_read; i++)
        {
            dblock_num = get_disk_block_from_inode_block(node, start_i_block + i, &prev_block);
            if(dblock_num <= 0)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Could not get disk block number for inode block %ld.\n", READ, start_i_block);
                altfs_free_memory(node);
                altfs_free_memory(buf_read);
                return -1;
            }

            buf_read = read_data_block(dblock_num);
            if(buf_read == NULL)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Could not read block for data block number %ld.\n", READ, dblock_num);
                altfs_free_memory(node);
                return -1;
            }

            if(i==0)
            {
                // For the 1st block, read only the contents after the start offset.
                memcpy(buff, buf_read + start_block_offset, BLOCK_SIZE - start_block_offset);
                bytes_read += BLOCK_SIZE - start_block_offset;
            }
            else if(i == blocks_to_read - 1)
            {
                // For the last block, read contents only till the end offset.
                memcpy(buff + bytes_read, buf_read, BLOCK_SIZE - end_block_offset);
                bytes_read += BLOCK_SIZE - end_block_offset;
            }
            else
            {
                memcpy(buff + bytes_read, buf_read, BLOCK_SIZE);
                bytes_read += BLOCK_SIZE;
            }
        }
    }
    altfs_free_memory(buf_read);
    time_t curr_time = time(NULL);
    node->i_atime = curr_time;

    if(!write_inode(inum, node)){
        fuse_log(FUSE_LOG_ERR, "%s : Could not write inode %ld.\n", READ, inum);
    }
    altfs_free_memory(node);
    return bytes_read;
}

ssize_t altfs_write(const char* path, void* buff, size_t nbytes, size_t offset)
{
    if(nbytes == 0)
    {
        return 0;
    }
    if(offset < 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Negative offset provided: %ld.\n", WRITE, offset);
        return -EINVAL;
    }

    ssize_t inum = name_i(path);
    if (inum == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Inode for file %s not found.\n", READ, path);
        return -ENOENT;
    }

    struct inode* node = get_inode(inum);
    size_t bytes_written=0;

    ssize_t start_i_block = offset / BLOCK_SIZE;
    ssize_t start_block_offset = offset % BLOCK_SIZE;
    ssize_t end_i_block = (offset + nbytes) / BLOCK_SIZE;
    ssize_t end_block_offset = BLOCK_SIZE - (offset + nbytes)% BLOCK_SIZE;
    ssize_t new_blocks_to_be_added = end_i_block - node->i_blocks_num + 1;

    char *buf_read = NULL;
    char overwrite_buf[BLOCK_SIZE];
    // First write to the data blocks that are allocated to the inode already.
    ssize_t prev_block = 0;
    for(ssize_t i = start_i_block; i < node->i_blocks_num; i++)
    {
        ssize_t dblock_num = get_disk_block_from_inode_block(node, i, &prev_block);
        if(dblock_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Could not get disk block number for inode block %ld.\n", WRITE, i);
            altfs_free_memory(node);
            return (bytes_written == 0) ? -1 : bytes_written;
        }

        bool written = false;
        if(i != start_i_block && i != end_i_block)
        {
            memcpy(overwrite_buf, buff + bytes_written, BLOCK_SIZE);
            bytes_written += BLOCK_SIZE;
            written = write_data_block(dblock_num, overwrite_buf)
        }
        else
        {
            buf_read = read_data_block(dblock_num);
            if(buf_read == NULL)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Could not read block for data block number %ld.\n", WRITE, dblock_num);
                altfs_free_memory(node);
                return (bytes_written == 0) ? -1 : bytes_written;
            }

            if(i == start_i_block)  // first block to be written
            {
                memcpy(buf_read + start_block_offset, buff, BLOCK_SIZE - start_block_offset);
                bytes_written += BLOCK_SIZE - start_block_offset;
            }
            else   // last block to be written (i == end_i_block)
            {
                memcpy(buf_read, buff + bytes_written, BLOCK_SIZE - end_block_offset);
                bytes_written += BLOCK_SIZE - end_block_offset;
                break;
            }
            written = write_data_block(dblock_num, buf_read);
        }

        if(!written)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Could not write data block number %ld.\n", WRITE, dblock_num);
            altfs_free_memory(buf_read);
            altfs_free_memory(node);
            return (bytes_written == 0) ? -1 : bytes_written;
        }
        altfs_free_memory(buf_read);
        buf_read = NULL;
    }

    ssize_t new_block_num;
    for(ssize_t i = 1; i <= new_blocks_to_be_added; i++)
    {
        new_block_num = allocate_data_block();
        if(new_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Could not allocate new data block. Bytes written %ld.\n", WRITE, bytes_written);
            break;
        }
        if(!add_datablock_to_inode(node, new_block_num))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Could not add new data block to inode %ld.\n", WRITE, inum);
            break;
        }

        if(i == new_blocks_to_be_added) // last block to be written
        {
            memset(overwrite_buf, 0, BLOCK_SIZE);
            memcpy(overwrite_buf, buff + bytes_written, BLOCK_SIZE - end_block_offset);
            ssize_t temp = BLOCK_SIZE - end_block_offset;
            bytes_written += temp;
        }
        else    // write entire block
        {
            memcpy(overwrite_buf, buff + bytes_written, BLOCK_SIZE);
            bytes_written += BLOCK_SIZE;
        }

        if(!write_data_block(new_block_num, buffer))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Could not write data block number %ld.\n", WRITE, new_block_num);
            break;
        }
    }
    altfs_free_memory(buffer);

    ssize_t bytes_to_add = (offset + bytes_written) - node->i_file_size;
    bytes_to_add = (bytes_to_add > 0) ? bytes_to_add : 0;
    node->i_file_size += bytes_to_add;
    if(bytes_written > 0)
    {
        time_t curr_time= time(NULL);
        node->i_mtime = curr_time;
        node->i_status_change_time = curr_time;
    }
    if(!write_inode(inum, node))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Could not write inode %ld.\n", WRITE, inum);
        return -1;
    }
    altfs_free_memory(node);
    return bytes_written;
}

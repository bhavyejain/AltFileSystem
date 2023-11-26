#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "../header/data_block_ops.h"
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
    if(!copy_child_file_name(child_name, path, path_len))
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
    if(!add_directory_entry(parent_inode, child_inode_num, child_name))
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
    if(!add_directory_entry(dir_inode, dir_inode_num, name))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to add directory entry: %s.\n", MKDIR, name);
        altfs_free_memory(dir_inode);
        return false;
    }

    name = "..";
    if(!add_directory_entry(dir_inode, parent_inum, name))
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
                free_memory(node);
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
    ssize_t inum = get_inode_num_from_path(path);
    if (inum == -1) {
        printf("ERROR in FILE_LAYER: Unable to find Inode for file %s\n", path);
        return -1;
    }

    struct iNode *inode = read_inode(inum);
    ssize_t bytes_to_add=0;

    if(offset + nbytes > inode->file_size){
        //add new blocks; 
        bytes_to_add = (offset + nbytes) - inode->file_size;
        ssize_t new_blocks_to_be_added = ((offset + nbytes) / BLOCK_SIZE) - inode->num_blocks + 1;
        // printf("Creating %ld new blocks for writing %ld bytes with %ld offset to inode %ld\n",
        //        new_blocks_to_be_added, nbytes, offset, inum);
        // DEBUG_PRINTF("Total new blocks being added to the file is %ld\n", new_blocks_to_be_added);
        ssize_t new_block_id;
        for(ssize_t i=0; i<new_blocks_to_be_added; i++){
            new_block_id = create_new_dblock();
            if(new_block_id<=0){
                printf("Failed to allocated new Data_Block for the write operation for the file %s\n", path);
                free_memory(inode);
                return -1;
            }
            if(!add_dblock_to_inode(inode, new_block_id)){
                printf("New Data Block addition to inode failed for file %s\n", path);
                free_memory(inode);
                return -1;
            }
        }
    }

    ssize_t start_block = offset / BLOCK_SIZE;
    ssize_t start_block_top_ceil = offset % BLOCK_SIZE;

    ssize_t end_block = (offset + nbytes) / BLOCK_SIZE;
    ssize_t end_block_bottom_floor = BLOCK_SIZE - (offset + nbytes)% BLOCK_SIZE;

    ssize_t nblocks_write = end_block - start_block + 1;

    // printf("start_block %ld, start_block_start %ld, end_block %ld, end_block_end %ld, blocks_to_write %ld\n",
    //         start_block, start_block_top_ceil, end_block, BLOCK_SIZE-end_block_bottom_floor, nblocks_write);

    DEBUG_PRINTF("writing %ld blocks to file\n", nblocks_write);

    size_t bytes_written=0;
    char *buf_read = NULL;

    // if there is only 1 block to write
    if(nblocks_write==1){
        ssize_t dblock_num = fblock_num_to_dblock_num(inode, start_block);
        if(dblock_num<=0){
            printf("Error getting dblocknum from Fblock_num during %ld block write for %s\n",start_block, path);
            free_memory(inode);
            return -1;
        }

        buf_read = read_dblock(dblock_num);
        if(buf_read == NULL){
            printf("Error encountered reading dblocknum %ld during %s write\n", dblock_num, path);
            free_memory(inode);
            return -1;
        }
        memcpy(buf_read+start_block_top_ceil, buff, nbytes);
        if(!write_dblock(dblock_num, buf_read)){
            printf("Error writing to dblock_num %ld during %s write", dblock_num, path);
            free_memory(buf_read);
            free_memory(inode);
            return -1;
        }
    }
    else{
        for(ssize_t i=0; i<nblocks_write; i++){
            ssize_t dblock_num = fblock_num_to_dblock_num(inode, start_block+i);
            if(dblock_num<=0){
                printf("Error in fetching the dblock_num from fblock_num %ld for %s\n", start_block+i, path);
                free_memory(inode);
                return -1;
            }
            buf_read=read_dblock(dblock_num);
            if(buf_read == NULL){
                printf("Error reading dblock_num %ld during the write of file %s\n",dblock_num, path);
                free_memory(inode);
                return -1;
            }
            // for the 1st block, start only after start_block_top_ceil
            if(i==0){
                memcpy(buf_read + start_block_top_ceil, buff, BLOCK_SIZE - (start_block_top_ceil));
                // printf("Added - %s\n", buf_read+start_block_top_ceil);
                bytes_written += BLOCK_SIZE-start_block_top_ceil;
            }
            else if(i==nblocks_write-1){
                //last block to be written
                memcpy(buf_read, buff + bytes_written, BLOCK_SIZE-end_block_bottom_floor);
                // printf("Added - %s\n", buf_read);
                bytes_written+= BLOCK_SIZE - end_block_bottom_floor;
            }
            else{
                memcpy(buf_read, buff + bytes_written, BLOCK_SIZE);
                // printf("Added - %s\n", buf_read);
                bytes_written += BLOCK_SIZE;
            }
            if(!write_dblock(dblock_num, buf_read)){
                printf("Error in writing to %ld dblock_num during the write of %s\n", dblock_num, path);
                free_memory(buf_read);
                free_memory(inode);
                return -1;
            }
            // printf("Bytes written %ld, Dblock %ld, Fblock %ld\n", bytes_written, dblock_num, start_block+i);
        }
    }
    free_memory(buf_read);
    inode->file_size += bytes_to_add;
    time_t curr_time= time(NULL);
    inode->access_time = curr_time;
    inode->modification_time = curr_time;
    inode->status_change_time = curr_time;
    if(!write_inode(inum, inode)){
        printf("Updating inode %ld for the file %s during the write operation failed\n",inum, path);
        return -1;
    }
    free_memory(inode);
    return nbytes;
}
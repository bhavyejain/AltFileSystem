#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include "../header/data_block_ops.h"
#include "../header/directory_ops.h"
#include "../header/inode_data_block_ops.h"
#include "../header/inode_ops.h"
#include "../header/interface_layer.h"
#include "../header/superblock_layer.h"

#define CREATE_NEW_FILE "create_new_file"

bool altfs_init()
{
    return setup_filesystem();
}

/*
struct stat {
    dev_t     st_dev;         ID of device containing file
    ino_t     st_ino;         Inode number
    mode_t    st_mode;        File type and mode
    nlink_t   st_nlink;       Number of hard links
    uid_t     st_uid;         User ID of owner
    gid_t     st_gid;         Group ID of owner
    dev_t     st_rdev;        Device ID (if special file)
    off_t     st_size;        Total size, in bytes
    blksize_t st_blksize;     Block size for filesystem I/O
    blkcnt_t  st_blocks;      Number of 512B blocks allocated
}
*/
static void inode_to_stat(struct inode** node, struct stat** st){
    // fuse_log(FUSE_LOG_DEBUG, "Filling st with inode info...\n");
    (*st)->st_mode = (*node)->i_mode;
    (*st)->st_nlink = (*node)->i_links_count;
    (*st)->st_uid = 0; // only one user for now
    (*st)->st_gid = 0; // only one group
    (*st)->st_rdev = 0;
    (*st)->st_size = (*node)->i_file_size;
    (*st)->st_blksize = BLOCK_SIZE;
    (*st)->st_blocks = (*node)->i_blocks_num;
    (*st)->st_atime = (*node)->i_atime;
    (*st)->st_ctime = (*node)->i_ctime;
    (*st)->st_mtime = (*node)->i_mtime;
    // fuse_log(FUSE_LOG_DEBUG, "Filling complete...\n");
}

ssize_t altfs_getattr(const char* path, struct stat** st)
{
    memset(*st, 0, sizeof(struct stat));
    ssize_t inum = name_i(path);
    if(inum == -1)
    {
        // fuse_log(FUSE_LOG_ERR, "%s : File %s not found.\n", GETATTR, path);
        return -ENOENT;
    }

    struct inode* node = get_inode(inum);
    if(node == NULL)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Inode for file %s not found.\n", GETATTR, path);
        return -EIO;
    }

    inode_to_stat(&node, st);
    (*st)->st_ino = inum;
    altfs_free_memory(node);

    // fuse_log(FUSE_LOG_DEBUG, "%s : Got attributes for %s\n", GETATTR, path);
    return 0;
}

ssize_t altfs_access(const char* path)
{
    ssize_t inum = name_i(path);
    if(inum == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : File %s not found.\n", ACCESS, path);
        return -ENOENT;
    }
    // TODO: Check Permissions
    fuse_log(FUSE_LOG_DEBUG, "%s : File %s found.\n", ACCESS, path);
    return 0;
}

/*
Allocates a new inode for a file and fills the inode with default values.
Make sure a file does not exist through name_i() before calling this.
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
    ssize_t child_inode_num = allocate_inode();
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
    fuse_log(FUSE_LOG_DEBUG, "%s : Created file %s\n", CREATE_NEW_FILE, path);
    return child_inode_num;
}

bool is_empty_dir(struct inode** dir_inode){
    if((*dir_inode)->i_child_num > 2)
    {
        // fuse_log(FUSE_LOG_DEBUG, "is_empty_dir : Directory has more than 2 entries: %ld.\n", (*dir_inode)->i_child_num);
        return false;
    }
    return true;
}

bool altfs_mkdir(const char* path, mode_t mode)
{
    struct inode* dir_inode = NULL;
    ssize_t parent_inum;

    ssize_t inum = name_i(path);
    if(inum != -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Directory already exists: %s.\n", MKDIR, path);
        return false;
    }

    ssize_t dir_inode_num = create_new_file(path, &dir_inode, S_IFDIR|mode, &parent_inum);
    if(dir_inode_num <= -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to allot inode with error: %ld.\n", MKDIR, dir_inode_num);
        altfs_free_memory(dir_inode);
        return false;
    }
    // fuse_log(FUSE_LOG_DEBUG, "%s : Alloted inode number %ld to directory %s.\n", MKDIR, dir_inode_num, path);

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

ssize_t altfs_readdir(const char* path, void* buff, fuse_fill_dir_t filler)
{
    ssize_t inum = name_i(path);
    if(inum == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Path %s not found.\n", READDIR, path);
        return -ENOENT;
    }

    struct inode* node = get_inode(inum);
    if(!S_ISDIR(node->i_mode))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Path %s is not a directory.\n", READDIR, path);
        return -ENOTDIR;
    }

    ssize_t num_blocks = node->i_blocks_num;
    ssize_t prev = 0;
    for(ssize_t i_block_num = 0; i_block_num < num_blocks; i_block_num++)
    {
        ssize_t dblock_num = get_disk_block_from_inode_block(node, i_block_num, &prev);
        char* dblock = read_data_block(dblock_num);
        
        ssize_t offset = 0;
        while(offset < LAST_POSSIBLE_RECORD)
        {
            char* record = dblock + offset;
            unsigned short rec_len = ((unsigned short*)record)[0];
            ssize_t file_inum = ((ssize_t*)(record + RECORD_LENGTH))[0];
            unsigned short name_len = rec_len - RECORD_FIXED_LEN;
            if(rec_len == 0)
                break;

            char file_name[name_len];
            memcpy(file_name, record + RECORD_FIXED_LEN, name_len);

            struct inode* file_inode = get_inode(file_inum);
            struct stat stbuff_data;
            memset(&stbuff_data, 0, sizeof(struct stat));
            struct stat *stbuff = &stbuff_data;
            inode_to_stat(&file_inode, &stbuff);
            stbuff->st_ino = file_inum;
            filler(buff, file_name, stbuff, 0, 0);  // check if the last value should be FUSE_FILL_DIR_PLUS
            altfs_free_memory(file_inode);

            record = NULL;
            offset += rec_len;
        }
        altfs_free_memory(dblock);
    }
    altfs_free_memory(node);

    return 0;
}

bool altfs_mknod(const char* path, mode_t mode, dev_t dev)
{
    // TODO: Buggy code
    dev = 0; // Silence error until used
    struct inode* node = NULL;

    // fuse_log(FUSE_LOG_DEBUG, "%s : MKNOD mode passed: %ld.\n", MKNOD, mode);

    ssize_t inum = name_i(path);
    if(inum != -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : File already exists: %s.\n", CREATE_NEW_FILE, path);
        return false;
    }

    ssize_t parent_inum;
    inum = create_new_file(path, &node, mode, &parent_inum);

    if (inum <= -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to allot inode with error: %ld.\n", MKNOD, inum);
        altfs_free_memory(node);
        return false;
    }

    altfs_free_memory(node);
    return true;
}

ssize_t altfs_unlink(const char* path)
{
    ssize_t inum = name_i(path);
    if(inum == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to get inode number for path: %s.\n", UNLINK, path);
        return -ENOENT;
    }
    if(inum == ROOT_INODE_NUM)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Cannot unlink root! Aborting.\n", UNLINK);
        return -EACCES;
    }

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

    struct inode* node = get_inode(inum);
    // If path is a directory which is not empty, fail operation
    if(S_ISDIR(node->i_mode) && !is_empty_dir(&node))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to unlink, dir is not empty: %s.\n", UNLINK, path);
        altfs_free_memory(node);
        return -ENOTEMPTY;
    }

    ssize_t parent_inum = name_i(parent_path);
    if(parent_inum == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to get inode number for parent path: %s.\n", UNLINK, parent_path);
        return -1;
    }
    struct inode* parent = get_inode(parent_inum);

    if(!remove_directory_entry(&parent, child_name))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Could not delete entry for child %s in parent %s.\n", UNLINK, child_name, parent_path);
        altfs_free_memory(node);
        altfs_free_memory(parent);
        return -1;
    }

    node->i_links_count--;
    if(node->i_links_count == 0)
    {
        remove_from_inode_cache(path);
        free_inode(inum);
    } else
    {
        time_t curr_time = time(NULL);
        node->i_status_change_time = curr_time;
        write_inode(inum, node);
    }
    write_inode(parent_inum, parent);
    altfs_free_memory(parent);
    altfs_free_memory(node);
    fuse_log(FUSE_LOG_DEBUG, "%s : Deleted file %s\n", UNLINK, path);
    return 0;
}

ssize_t altfs_open(const char* path, ssize_t oflag)
{
    ssize_t inum = name_i(path);
    bool created = false;

    // Create new file if required
    if(inum == -1 && (oflag & O_CREAT))
    {
        struct inode* node = NULL;
        ssize_t parent_inum;
        inum = create_new_file(path, &node, S_IFREG|DEFAULT_PERMISSIONS, &parent_inum);
        if(inum <= -1)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Error creating file %s, errno: %ld.\n", OPEN, path, inum);
            return inum;
        }
        // write_inode(inum, node);
        altfs_free_memory(node);
        created = true;
    }
    else if(inum == -1)
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
    if((oflag & O_WRONLY) || (oflag & O_RDWR)) // Needs write permission
    {
        if(!(bool)(node->i_mode & S_IWUSR))
        {
            fuse_log(FUSE_LOG_ERR, "%s : File %s does not have write permission.\n", OPEN, path);
            return -EACCES;
        }
    }

    // Truncate if required
    if((bool)(oflag & O_TRUNC) && (bool)(node->i_mode & S_IWUSR))
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
    fuse_log(FUSE_LOG_DEBUG, "%s : Opened file %s\n", OPEN, path);
    return inum;
}

// Check if this even needs to be implemented.
ssize_t altfs_close(ssize_t file_descriptor)
{
    return 0;
}

ssize_t altfs_read(const char* path, char* buff, size_t nbytes, off_t offset)
{
    fuse_log(FUSE_LOG_DEBUG, "%s : Attempting to read %ld bytes from %s at offset %ld.\n", READ, nbytes, path, offset);
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
    if(!(bool)(node->i_mode & S_IRUSR))
    {
        fuse_log(FUSE_LOG_ERR, "%s : File %s does not have read permission.\n", READ, path);
        return -EACCES;
    }
    if (node->i_file_size == 0)
    {
        fuse_log(FUSE_LOG_DEBUG, "%s : File size 0, read 0 bytes from %s.\n", READ, path);
        return 0;
    }
    if (offset >= node->i_file_size)
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
    ssize_t end_i_block = (offset + nbytes - 1) / BLOCK_SIZE; // Last logical block to read from
    ssize_t end_block_offset = ((offset + nbytes - 1) % BLOCK_SIZE); // Ending offet in last block till where to read

    ssize_t blocks_to_read = end_i_block - start_i_block + 1; // Number of blocks that need to be read
    // fuse_log(FUSE_LOG_DEBUG, "%s : Number of blocks to read: %ld.\n", READ, blocks_to_read);

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

            if(i == 0)
            {
                // For the 1st block, read only the contents after the start offset.
                ssize_t bytes_to_read = BLOCK_SIZE - start_block_offset;
                memcpy(buff, buf_read + start_block_offset, bytes_to_read);
                bytes_read += bytes_to_read;
            }
            else if(i == blocks_to_read - 1)
            {
                // For the last block, read contents only till the end offset.
                memcpy(buff + bytes_read, buf_read, end_block_offset + 1);
                bytes_read += (end_block_offset + 1);
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
    fuse_log(FUSE_LOG_DEBUG, "%s : Read %ld bytes from %s.\n", READ, bytes_read, path);
    return bytes_read;
}

ssize_t altfs_write(const char* path, const char* buff, size_t nbytes, off_t offset)
{
    fuse_log(FUSE_LOG_DEBUG, "%s : Attempting to write %ld bytes to %s at offset %ld.\n", WRITE, nbytes, path, offset);
    if(nbytes == 0)
    {
        fuse_log(FUSE_LOG_DEBUG, "%s : Nbytes is 0, returning 0.\n", WRITE);
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
        fuse_log(FUSE_LOG_ERR, "%s : Inode for file %s not found.\n", WRITE, path);
        return -ENOENT;
    }

    struct inode* node = get_inode(inum);
    if(!(bool)(node->i_mode & S_IWUSR))
    {
        fuse_log(FUSE_LOG_ERR, "%s : File %s does not have write permission.\n", WRITE, path);
        return -EACCES;
    }
    size_t bytes_written = 0;

    ssize_t start_i_block = (ssize_t)(offset / BLOCK_SIZE);
    ssize_t start_block_offset = (ssize_t)(offset % BLOCK_SIZE);
    ssize_t end_i_block = (ssize_t)((offset + nbytes - 1) / BLOCK_SIZE);
    ssize_t end_block_offset = (ssize_t)((offset + nbytes - 1) % BLOCK_SIZE);
    ssize_t new_blocks_to_be_added = end_i_block - node->i_blocks_num + 1;
    ssize_t starting_block = start_i_block - node->i_blocks_num + 1; // In case offset > file size, we might be starting some blocks after what has been allocated.

    char overwrite_buf[BLOCK_SIZE];
    // First write to the data blocks that are allocated to the inode already.
    ssize_t prev_block = 0;
    bool complete = false;
    for(ssize_t i = start_i_block; i < node->i_blocks_num && !complete; i++)
    {
        char *buf_read = NULL;
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
            // fuse_log(FUSE_LOG_DEBUG, "%s : Loop 1 mid block, writing %ld bytes.\n", WRITE, BLOCK_SIZE);
            memcpy(overwrite_buf, buff + bytes_written, BLOCK_SIZE);
            bytes_written += BLOCK_SIZE;
            written = write_data_block(dblock_num, overwrite_buf);
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

            if(i == start_i_block)  // first block to be written (i == start_i_block)
            {
                ssize_t to_write = ((start_block_offset + nbytes) > BLOCK_SIZE) ? (BLOCK_SIZE - start_block_offset) : nbytes;
                // fuse_log(FUSE_LOG_DEBUG, "%s : Loop 1 start block, writing %ld bytes.\n", WRITE, to_write);
                memcpy(buf_read + start_block_offset, buff, to_write);
                bytes_written += to_write;
            }
            else   // last block to be written 
            {
                // fuse_log(FUSE_LOG_DEBUG, "%s : Loop 1 end block, writing %ld bytes.\n", WRITE, (end_block_offset + 1));
                memcpy(buf_read, buff + bytes_written, end_block_offset + 1);
                bytes_written += (end_block_offset + 1);
            }
            complete = (i == end_i_block) ? true : false;

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

        memset(overwrite_buf, 0, BLOCK_SIZE);
        if(starting_block >= 1 && i == starting_block) // first block with data; only goes in if overall starts writing here
        {
            ssize_t to_write = ((start_block_offset + nbytes) > BLOCK_SIZE) ? (BLOCK_SIZE - start_block_offset) : nbytes;
            // fuse_log(FUSE_LOG_DEBUG, "%s : Loop 2 start block, writing %ld bytes.\n", WRITE, to_write);
            memcpy(overwrite_buf + start_block_offset, buff, to_write);
            bytes_written += to_write;
        }
        else if(i == new_blocks_to_be_added) // last block to be written
        {
            // fuse_log(FUSE_LOG_DEBUG, "%s : Loop 2 end block, writing %ld bytes.\n", WRITE, (end_block_offset + 1));
            memcpy(overwrite_buf, buff + bytes_written, end_block_offset + 1);
            bytes_written += (end_block_offset + 1);
        }
        else if(i >= starting_block)    // write entire block
        {
            // fuse_log(FUSE_LOG_DEBUG, "%s : Loop 2 mid block, writing %ld bytes.\n", WRITE, BLOCK_SIZE);
            memcpy(overwrite_buf, buff + bytes_written, BLOCK_SIZE);
            bytes_written += BLOCK_SIZE;
        }

        if((i >= starting_block) && !write_data_block(new_block_num, overwrite_buf))
        {
            fuse_log(FUSE_LOG_ERR, "%s : Could not write data block number %ld.\n", WRITE, new_block_num);
            break;
        }
    }

    ssize_t bytes_to_add = (ssize_t)((offset + bytes_written) - node->i_file_size);
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
    fuse_log(FUSE_LOG_DEBUG, "%s : Written %ld bytes to %s\n", WRITE, bytes_written, path);
    altfs_free_memory(node);
    return bytes_written;
}

ssize_t altfs_truncate(const char* path, off_t length)
{
    // fuse_log(FUSE_LOG_DEBUG, "%s : Truncating %s to %ld bytes.\n", TRUNCATE, path, (ssize_t)length);
    ssize_t inum = name_i(path);
    if(inum == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to get inode number for path: %s.\n", TRUNCATE, path);
        return -ENOENT;
    }
    struct inode* node = get_inode(inum);
    if(!(bool)(node->i_mode & S_IWUSR))
    {
        fuse_log(FUSE_LOG_ERR, "%s : File %s does not have write permission.\n", TRUNCATE, path);
        return -EACCES;
    }

    if(length < 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Negative legth provided: %ld.\n", TRUNCATE, length);
        return -EINVAL;
    }
    
    if(length == 0 && node->i_file_size == 0)
    {
        fuse_log(FUSE_LOG_DEBUG, "%s : %s Offset is 0 and file size is also 0.\n", TRUNCATE, path);
        altfs_free_memory(node);
        return 0;
    }

    if(length > node->i_file_size)
    {
        ssize_t bytes_to_add = length - node->i_file_size;
        char* zeros = (char*)calloc(1, bytes_to_add);
        ssize_t written = altfs_write(path, zeros, bytes_to_add, node->i_file_size);
        altfs_free_memory(node);
        if(written == bytes_to_add)
        {
            fuse_log(FUSE_LOG_DEBUG, "%s : Extended %s by %ld bytes.\n", TRUNCATE, path, written);
            return 0;
        } else
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to truncate.\n", TRUNCATE);
            return -1;
        }
    }

    ssize_t i_block_num = (length == 0) ? -1 : (ssize_t)((length - 1) / BLOCK_SIZE);
    if(node->i_blocks_num > i_block_num + 1)
    {
        // Remove everything from i_block_num + 1
        remove_datablocks_from_inode(node, i_block_num + 1);
    }

    if(i_block_num == -1)
    {
        node->i_file_size = 0;
        write_inode(inum, node);
        altfs_free_memory(node);
        fuse_log(FUSE_LOG_DEBUG, "%s : Truncated %s to 0 bytes\n", TRUNCATE, path);
        return 0;
    }

    // Get data block for offset
    ssize_t prev = 0;
    ssize_t d_block_num = get_disk_block_from_inode_block(node, i_block_num, &prev);
    if(d_block_num <= 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Failed to get data block number from inode block number.\n", TRUNCATE);
        return -1;
    }
    char* data_block = read_data_block(d_block_num);

    ssize_t block_offset = (ssize_t)((length - 1) % BLOCK_SIZE);
    if(block_offset != BLOCK_SIZE - 1)
    {
        memset(data_block + block_offset + 1, 0, BLOCK_SIZE - block_offset - 1);
        write_data_block(d_block_num, data_block);
    }
    altfs_free_memory(data_block);

    node->i_file_size = (ssize_t)length;
    write_inode(inum, node);
    altfs_free_memory(node);
    fuse_log(FUSE_LOG_DEBUG, "%s : Truncated %s to %ld bytes.\n", TRUNCATE, path, (ssize_t)length);
    return 0;
}

ssize_t altfs_chmod(const char* path, mode_t mode)
{
    ssize_t inum = name_i(path);
    if(inum == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : File %s not found.\n", CHMOD, path);
        return -ENOENT;
    }

    struct inode* node = get_inode(inum);
    if(node == NULL)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Inode read returned null.\n", CHMOD);
        return -1;
    }

    node->i_mode = mode;
    if(!write_inode(inum, node))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Inode write unsuccessful.\n", CHMOD);
        return -1;
    }
    return 0;
}

ssize_t altfs_rename(const char *from, const char *to)
{
    // fuse_log(FUSE_LOG_DEBUG, "%s : Attempting rename from %s to %s.\n", RENAME, from, to);
    // Hacky approach - copy file to the new location and remove the old one
    // check if from exists

    if(altfs_access(from) != 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : From path %s not found.\n", RENAME, from);
        return -ENOENT;
    }
    // check if to exists
    if(altfs_access(to) == 0)
    {
        // to exists, try to unlink it
        ssize_t unlink_res = altfs_unlink(to);
        if(unlink_res != 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to unlink %s\n", RENAME, to);
            return unlink_res;
        }
    }
    
    ssize_t to_path_len = strlen(to);
    char to_parent_path[to_path_len + 1];
    if(!copy_parent_path(to_parent_path, to, to_path_len))
    {
        fuse_log(FUSE_LOG_ERR, "%s : No parent path exists for to path: %s.\n", RENAME, to);
        return -1;
    }

    // Check for to's parent sanity.
    ssize_t to_parent_inode_num = name_i(to_parent_path);
    if(to_parent_inode_num == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Invalid to parent path: %s.\n", RENAME, to_parent_path);
        return -1;
    }
    struct inode* to_parent_inode = get_inode(to_parent_inode_num);
    if(!S_ISDIR(to_parent_inode->i_mode))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Parent is not a directory: %s.\n", RENAME, to_parent_path);
        altfs_free_memory(to_parent_inode);
        return -1;
    }
    // Check parent write permission
    // if(!(bool)(to_parent_inode->i_mode & S_IWUSR))
    // {
    //     fuse_log(FUSE_LOG_ERR, "%s : Parent directory does not have write permission: %s.\n", RENAME, to_parent_path);
    //     altfs_free_memory(to_parent_inode);
    //     return -EACCES;
    // }

    /*
    Add record in to's parent.
    */
    // fuse_log(FUSE_LOG_DEBUG, "%s : Adding record in TO's parent.\n", RENAME);
    char to_child_name[to_path_len + 1];
    if(!copy_file_name(to_child_name, to, to_path_len))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Error getting child file name from path: %s.\n", RENAME, to);
        return -1;
    }

    ssize_t inum = name_i(from);
    if(!add_directory_entry(&to_parent_inode, inum, to_child_name))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Error adding child record %s to parent %s: %s.\n", RENAME, to_child_name, to_parent_path);
        altfs_free_memory(to_parent_inode);
        return -EDQUOT;
    }
    time_t curr_time = time(NULL);
    to_parent_inode->i_mtime = curr_time;
    to_parent_inode->i_ctime = curr_time;
    if(!write_inode(to_parent_inode_num, to_parent_inode))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Could not write directory inode.\n", RENAME);
        altfs_free_memory(to_parent_inode);
        return -1;
    }
    altfs_free_memory(to_parent_inode);

    /*
    Remove record in from's parent.
    */
    // fuse_log(FUSE_LOG_DEBUG, "%s : Removing record from FROM's parent.\n", RENAME);
    ssize_t from_path_len = strlen(from);
    char from_parent_path[from_path_len + 1];
    if(!copy_parent_path(from_parent_path, from, from_path_len))
    {
        fuse_log(FUSE_LOG_ERR, "%s : No parent path exists for path: %s.\n", RENAME, from);
        return -1;
    }

    ssize_t from_parent_inode_num = name_i(from_parent_path);
    if(from_parent_inode_num == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Invalid parent path: %s.\n", RENAME, from_parent_path);
        return -1;
    }
    struct inode* from_parent_inode = get_inode(from_parent_inode_num);

    char from_child_name[from_path_len + 1];
    if(!copy_file_name(from_child_name, from, from_path_len))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Error getting child file name from path: %s.\n", RENAME, from);
        return -1;
    }

    if(!remove_directory_entry(&from_parent_inode, from_child_name))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Could not delete entry for child %s in parent %s.\n", RENAME, from_child_name, from_parent_path);
        altfs_free_memory(from_parent_inode);
        return -1;
    }
    from_parent_inode->i_mtime = curr_time;
    from_parent_inode->i_ctime = curr_time;
    if(!write_inode(from_parent_inode_num, from_parent_inode))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Could not write directory inode.\n", RENAME);
        altfs_free_memory(from_parent_inode);
        return -1;
    }
    altfs_free_memory(from_parent_inode);

    flush_inode_cache();

    fuse_log(FUSE_LOG_DEBUG, "%s : Renamed %s to %s\n", RENAME, from, to);
    return 0;
}

void altfs_destroy()
{
    teardown();
}

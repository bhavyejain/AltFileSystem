#include <time.h>

#include "../header/superblock_layer.h"
#include "../header/inode_ops.h"
#include "../header/data_block_ops.h"
#include "../header/directory_ops.h"
#include "../header/inode_cache.h"
#include "../header/inode_data_block_ops.h"

static struct inode_cache inodeCache;

ssize_t get_last_index_of_parent_path(const char* const path, ssize_t path_length)
{
     // TODO: Ignore multiple /// in path
    for(ssize_t i = path_length-1 ; i >= 0 ; i--)
    {
        if (path[i] == '/' && i != path_length-1)
            return i;
    }
    return -1;
}

bool copy_parent_path(char* const buffer, const char* const parent_path, ssize_t path_len)
{
    ssize_t index = get_last_index_of_parent_path(parent_path, path_len);
    if(index == -1){
        return false;
    }
    memcpy(buffer, parent_path, index + 1);
    // We add null char since buffer length will be > than path len
    buffer[index + 1] = '\0';
    return true;
} 

bool copy_file_name(char* const buffer, const char* const path, ssize_t path_len)
{
    ssize_t start_index = get_last_index_of_parent_path(path, path_len), end_index = path_len;;
    
    if(start_index == -1)
    {
        return false;
    }

    start_index++;
    
    // remove trailing /
    if(path[end_index-1]=='/'){
        end_index--;
    }
    memcpy(buffer, path+start_index, end_index-start_index);
    buffer[end_index - start_index]='\0';
    return true;
}

/*
ALGORTIHM:

If directory entry has allocated datablocks:
    Iterate over logical data blocks serially [i]:
        Read data block, and set curr_pos to 0
        While curr_pos < (BLOCK_SIZE - RECORD_FIXED_SIZE):
            Read the record_len bytes
            If record_len == 0:  // means we are past existing records for the data block
                Add new record here if enough space left, and return
            Increment curr_pos by record_len
Add new datablock to the inode
Write the reord to the new block
Return
*/
bool add_directory_entry(struct inode** dir_inode, ssize_t child_inum, char* file_name)
{
    // Check if dir_inode is actually a directory
    if(!S_ISDIR((*dir_inode)->i_mode))
    {
        fuse_log(FUSE_LOG_ERR, "%s : The parent inode is not a directory.\n", ADD_DIRECTORY_ENTRY);
        return false;
    }

    // Check for maximum lenth of file name
    ssize_t file_name_len = strlen(file_name);
    if(file_name_len > MAX_FILE_NAME_LENGTH)
    {
        fuse_log(FUSE_LOG_ERR, "%s : File name is > 255 bytes.\n", ADD_DIRECTORY_ENTRY);
        return false;
    }
    file_name_len++; // add \0

    unsigned short short_name_length = file_name_len;

    // If the directory inode has datablocks allocated, try to find space in them to add the entry.
    if((*dir_inode)->i_blocks_num > 0)
    {
        ssize_t prev_block = 0;
        for(ssize_t l_block_num = 0; l_block_num < (*dir_inode)->i_blocks_num; l_block_num++)
        {
            ssize_t p_block_num = get_disk_block_from_inode_block((*dir_inode), l_block_num, &prev_block);
            if(p_block_num <= 0)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to fetch physical data block number corresponfing to file's logical block number.\n", ADD_DIRECTORY_ENTRY);
                return false;
            }

            char* dblock = read_data_block(p_block_num);
            ssize_t curr_pos = 0;
            while(curr_pos <= LAST_POSSIBLE_RECORD)
            {
                unsigned short record_len = ((unsigned short*)(dblock + curr_pos))[0];

                // Indicates a record has never been allocated at (and from) this position
                if(record_len == 0)
                {
                    // Check if there is enough space left in the data block
                    ssize_t rem_block_space = BLOCK_SIZE - curr_pos - RECORD_FIXED_LEN;
                    if(rem_block_space >= file_name_len)
                    {
                        // UNCOMMENT
                        // fuse_log(FUSE_LOG_DEBUG, "%s : Space found in data block %ld (physical block #%ld) of directory for entry.\n", ADD_DIRECTORY_ENTRY, l_block_num, p_block_num);
                        unsigned short record_length = RECORD_FIXED_LEN + short_name_length;
                        char* record = dblock + curr_pos;
                        // Add record length
                        ((unsigned short*)record)[0] = record_length;
                        // Add child INUM
                        ((ssize_t*)(record + RECORD_LENGTH))[0] = child_inum;
                        // Add file name
                        strncpy((char*)(record + RECORD_FIXED_LEN), file_name, file_name_len);

                        if(!write_data_block(p_block_num, dblock)){
                            fuse_log(FUSE_LOG_ERR, "%s : Error writing data block %ld to disk.\n", ADD_DIRECTORY_ENTRY, p_block_num);
                            altfs_free_memory(dblock);
                            return false;
                        }

                        (*dir_inode)->i_child_num++;
                        altfs_free_memory(dblock);
                        return true;
                    } else
                    {
                        break;
                    }
                }

                curr_pos += record_len;
            }

            altfs_free_memory(dblock);
        }
    }
    fuse_log(FUSE_LOG_DEBUG, "%s : No space found in existing data blocks for directory entry, allocating a new block.\n", ADD_DIRECTORY_ENTRY);

    // Add a data block to the inode
    ssize_t data_block_num = allocate_data_block();
    if(data_block_num <= 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Error allocating new data block for directory entry.\n", ADD_DIRECTORY_ENTRY);
        return false;
    }

    char data_block[BLOCK_SIZE];
    memset(data_block, 0, BLOCK_SIZE);

    unsigned short record_length = RECORD_FIXED_LEN + short_name_length;
    // Add record length
    ((unsigned short*)data_block)[0] = record_length;
    // Add child INUM
    ((ssize_t*)(data_block + RECORD_LENGTH))[0] = child_inum;
    // Add file name
    strncpy((char*)(data_block + RECORD_FIXED_LEN), file_name, file_name_len);

    if(!write_data_block(data_block_num, data_block))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Error writing data block %ld to disk.\n", ADD_DIRECTORY_ENTRY, data_block_num);
        return false;
    }

    if(!add_datablock_to_inode((*dir_inode), data_block_num))
    {
        printf("couldn't add dblock to inode\n");
        return false;
    }

    (*dir_inode)->i_file_size += BLOCK_SIZE;   // TODO: Should this be in the add datablock to inode function?
    (*dir_inode)->i_child_num++;
    return true;
}

struct fileposition get_file_position_in_dir(const char* const file_name, const struct inode* const parent_inode)
{
    // fuse_log(FUSE_LOG_ERR, "%s : Looking for file: %s.\n", GET_FILE_POS_IN_DIR, file_name);
    struct fileposition filepos;
    filepos.offset = -1;
    filepos.p_block = NULL;
    filepos.p_plock_num = -1;

    if (!S_ISDIR(parent_inode->i_mode))
    {
        fuse_log(FUSE_LOG_ERR, "%s : The parent inode is not a directory.\n", GET_FILE_POS_IN_DIR);
        return filepos;
    }

    // Check for maximum lenth of file name
    ssize_t file_name_len = strlen(file_name);
    if(file_name_len > MAX_FILE_NAME_LENGTH){
        fuse_log(FUSE_LOG_ERR, "%s : File name is > 255 bytes.\n", GET_FILE_POS_IN_DIR);
        return filepos;
    }

    ssize_t prev_block = 0;
    for(ssize_t l_block_num = 0; l_block_num < parent_inode->i_blocks_num; l_block_num++)
    {
        filepos.p_plock_num = get_disk_block_from_inode_block(parent_inode, l_block_num, &prev_block);

        if(filepos.p_plock_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to fetch physical data block number corresponfing to file's logical block number.\n", GET_FILE_POS_IN_DIR);
            return filepos;
        }

        filepos.p_block = read_data_block(filepos.p_plock_num);

        // traverse the data block to find an inode entry with the given file name
        ssize_t curr_pos = 0;
        while(curr_pos <= LAST_POSSIBLE_RECORD)
        {
            unsigned short record_len = ((unsigned short*)(filepos.p_block + curr_pos))[0];
            char* curr_file_name = filepos.p_block + curr_pos + RECORD_FIXED_LEN;
            unsigned short curr_file_name_len = ((unsigned short)(record_len - RECORD_FIXED_LEN - 1));  // adjust for \0
            // fuse_log(FUSE_LOG_ERR, "%s : Current record => file name: %s, rec_len: %d.\n", GET_FILE_POS_IN_DIR, curr_file_name, record_len);

            // If record len = 0 => we are past existing records for the data block, we can move to the next data block
            if (record_len == 0)
                break;

            // If the file name matches the input file name => we have found our file
            if (curr_file_name_len == strlen(file_name) && 
                strncmp(curr_file_name, file_name, curr_file_name_len) == 0) {
                // fuse_log(FUSE_LOG_ERR, "%s : Match for file name: %s in physical block %ld.\n", GET_FILE_POS_IN_DIR, curr_file_name, filepos.p_plock_num);
                filepos.offset = curr_pos;
                return filepos;
            }
            else {
                curr_pos += record_len;
            }
        } 
    }
    return filepos;
}

ssize_t name_i(const char* const file_path)
{
    ssize_t file_path_len = strlen(file_path);

    if (file_path_len == 1 && file_path[0] == '/')
    {
        fuse_log(FUSE_LOG_DEBUG, "%s : Path is /. Returning root inum\n", NAME_I);
        return ROOT_INODE_NUM;
    }

    // Check for presence in cache
    ssize_t inum_from_cache = get_cache_entry(&inodeCache, file_path);
    if (inum_from_cache > 0)
    {
        fuse_log(FUSE_LOG_DEBUG, "%s : Returning inum %ld for %s from cache.\n", NAME_I, inum_from_cache, file_path);
        return inum_from_cache;
    }

    // Recursively get inum from parent
    char parent_path[file_path_len + 1];
    if (!copy_parent_path(parent_path, file_path, file_path_len))
        return -1;
    
    fuse_log(FUSE_LOG_DEBUG, "%s : Parent for %s : %s\n", NAME_I, file_path, parent_path);
    
    ssize_t parent_inum = name_i(parent_path);
    if (parent_inum == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Parent %ld not found.\n", NAME_I, parent_inum);
        return -1;
    }

    struct inode* inodeObj = get_inode(parent_inum);
    char child_path[file_path_len + 1];
    if(!copy_file_name(child_path, file_path, file_path_len)){
        altfs_free_memory(inodeObj);
        return -1;
    }

    // find the position of the file in the dir
    struct fileposition filepos = get_file_position_in_dir(child_path, inodeObj);
    altfs_free_memory(inodeObj);
    
    if(filepos.offset == -1){
        altfs_free_memory(filepos.p_block);
        return -1;
    }

    ssize_t inum = ((ssize_t*) (filepos.p_block + filepos.offset + RECORD_LENGTH))[0];
    
    altfs_free_memory(filepos.p_block);
    set_cache_entry(&inodeCache, file_path, inum);
    fuse_log(FUSE_LOG_DEBUG, "%s : Added cache entry %ld for %s.\n", NAME_I, inum, file_path);
    return inum;
}

// Run makefs() before running initialize
bool setup_filesystem()
{
    if(!load_superblock())
    {
        fuse_log(FUSE_LOG_ERR, "%s : No superblock found for altfs, aborting initialization!\n", SETUP_FILESYSTEM);
        return false;
    }

    // Create a cache that can be used to implement namei
    create_inode_cache(&inodeCache, CACHE_CAPACITY);
    fuse_log(FUSE_LOG_DEBUG, "%s : Created inode cache to retrieve inode data faster.\n", SETUP_FILESYSTEM);

    // Check for root directory
    struct inode* root_dir = get_inode(ROOT_INODE_NUM);
    if(root_dir == NULL){
        fuse_log(FUSE_LOG_ERR, "%s : The root inode is null\n", SETUP_FILESYSTEM);
        return false;
    }

    if(root_dir->i_allocated && root_dir->i_child_num >= 2)
    {
        fuse_log(FUSE_LOG_DEBUG, "%s : Root directory found! Initialization complete.", SETUP_FILESYSTEM);
        return true;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s : Root directory not found, creating root...", SETUP_FILESYSTEM);

    time_t curr_time = time(NULL);

    root_dir->i_allocated = true;
    root_dir->i_links_count += 1;
    root_dir->i_mode = S_IFDIR | DEFAULT_PERMISSIONS;
    root_dir->i_blocks_num = 0;
    root_dir->i_file_size = 0;
    root_dir->i_atime = curr_time;
    root_dir->i_ctime = curr_time;
    root_dir->i_mtime = curr_time;
    root_dir->i_status_change_time = curr_time;
    root_dir->i_child_num = 0;

    char* dir_name = ".";
    if(!add_directory_entry(&root_dir, ROOT_INODE_NUM, dir_name)){
        fuse_log(FUSE_LOG_ERR, "%s Failed to add . entry for root directory\n", SETUP_FILESYSTEM);
        return false;
    }

    fuse_log(FUSE_LOG_DEBUG, "%s Successfully added . entry for root directory\n", SETUP_FILESYSTEM);

    dir_name = "..";
    if(!add_directory_entry(&root_dir, ROOT_INODE_NUM, dir_name)){
        fuse_log(FUSE_LOG_ERR, "%s Failed to add .. entry for root directory\n", SETUP_FILESYSTEM);
        return false;
    }

    fuse_log(FUSE_LOG_DEBUG, "%s Successfully added .. entry for root directory\n", SETUP_FILESYSTEM);

    if(!write_inode(ROOT_INODE_NUM, root_dir)){
        fuse_log(FUSE_LOG_ERR, "%s Failed to write inode for root directory\n", SETUP_FILESYSTEM);
        // TODO: remove directory entries?
        return false;
    }
    
    fuse_log(FUSE_LOG_DEBUG, "%s Successfully wrote root dir inode with %ld data blocks\n", SETUP_FILESYSTEM, root_dir->i_blocks_num);
    altfs_free_memory(root_dir);

    return true;
}

bool remove_from_inode_cache(const char* path)
{
    if(!remove_cache_entry(&inodeCache, path))
    {
        return false;
    }
    return true;
}

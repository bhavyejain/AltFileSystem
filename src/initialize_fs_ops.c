#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <fuse.h>

#include "../header/superblock_layer.h"
#include "../header/disk_layer.h"
#include "../header/inode_ops.h"
#include "../header/data_block_ops.h"
#include "../header/initialize_fs_ops.h"
#include "../header/inode_cache.h"

static struct inode_cache inodeCache;

/*
Get the physical disk block number for a given file and logical block number in the file.

@param file_inode: Constant pointer to the file's inode
@param file_block_num: Logical block number in the file

@return The physical data block number.
*/
ssize_t get_data_block_from_file_block(const struct inode* const file_inode, ssize_t file_block_num)
{
    ssize_t data_block_num = -1;

    if(file_block_num > file_inode->i_blocks_num)
    {
        fuse_log(FUSE_LOG_ERR, "%s : File block number %ld is greater than data block count of file inode with %ld data blocks\n", GET_DBLOCK_FROM_FBLOCK, file_block_num, file_inode->i_blocks_num);
        return data_block_num;
    }
    
    // If file block is within direct block count, return data block number directly
    if(file_block_num < NUM_OF_DIRECT_BLOCKS)
    {
        data_block_num = file_inode->i_direct_blocks[file_block_num];
        fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from direct block\n", GET_DBLOCK_FROM_FBLOCK, data_block_num);
        return data_block_num;
    }

    // Adjust logical block number for single indirect
    file_block_num -= NUM_OF_DIRECT_BLOCKS;

    // If file block num < 512 => single indirect block
    if(file_block_num < NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)
    {
        if(file_inode->i_single_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR,"%s : Single indirect block is set to 0 for inode.\n", GET_DBLOCK_FROM_FBLOCK);
            return data_block_num;
        }

        // Read single indirect block and extract data block num from file block num
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(file_inode->i_single_indirect);
        data_block_num = single_indirect_block_arr[file_block_num];
        altfs_free_memory(single_indirect_block_arr);

        fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from single indirect block\n", GET_DBLOCK_FROM_FBLOCK, data_block_num);
        return data_block_num;
    }

    // Adjust logical block number for double indirect
    file_block_num -= NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;
    
    // If file block num < 512*512 => double indirect block
    if(file_block_num < NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR)
    {
        if(file_inode->i_double_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR,"%s : Double indirect block is set to 0 for inode.\n", GET_DBLOCK_FROM_FBLOCK);
            return data_block_num;
        }
        ssize_t double_i_idx = file_block_num / NUM_OF_ADDRESSES_PER_BLOCK;
        ssize_t inner_idx = file_block_num % NUM_OF_ADDRESSES_PER_BLOCK;

        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(file_inode->i_double_indirect);
        data_block_num = double_indirect_block_arr[double_i_idx];
        altfs_free_memory(double_indirect_block_arr);

        if(data_block_num <= 0){
            fuse_log(FUSE_LOG_ERR, "%s : Double indirect block num <= 0.\n", GET_DBLOCK_FROM_FBLOCK);
            return -1;
        }

        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);
        data_block_num = single_indirect_block_arr[inner_idx];
        altfs_free_memory(single_indirect_block_arr);

        fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from double indirect block\n", GET_DBLOCK_FROM_FBLOCK, data_block_num);
        return data_block_num
    }

    // Adjust logical block number for double indirect
    file_block_num -= NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;

    // If file block num < 512*512*512 => triple indirect block
    if(file_inode->i_triple_indirect == 0)
    {
        fuse_log(FUSE_LOG_ERR,"%s : Triple indirect block is set to 0 for inode. Exiting\n", GET_DBLOCK_FROM_FBLOCK);
        return data_block_num;
    }

    ssize_t triple_i_idx = file_block_num / NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;
    ssize_t double_i_idx = (file_block_num / NUM_OF_ADDRESSES_PER_BLOCK) % NUM_OF_ADDRESSES_PER_BLOCK;
    ssize_t inner_idx = file_block_num % NUM_OF_ADDRESSES_PER_BLOCK;

    ssize_t* triple_indirect_block_arr = (ssize_t*) read_data_block(file_inode->i_triple_indirect);
    data_block_num = triple_indirect_block_arr[triple_i_idx];
    altfs_free_memory(triple_indirect_block_arr);

    if(data_block_num <= 0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Triple indirect block num <= 0.\n", GET_DBLOCK_FROM_FBLOCK);
        return -1;
    }
    
    ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);
    data_block_num = double_indirect_block_arr[double_i_idx];
    altfs_free_memory(double_indirect_block_arr);
    
    if(data_block_num<=0)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Double indirect block num <= 0. Exiting\n", GET_DBLOCK_FROM_FBLOCK);
        return -1;
    }

    ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);
    data_block_num = single_indirect_block_arr[inner_idx];
    altfs_free_memory(single_indirect_block_arr);

    fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from triple indirect block\n", GET_DBLOCK_FROM_FBLOCK, data_block_num);
    return data_block_num;
}

/*
A directory entry (record) in altfs looks like:
| Total entry length (2) | Allocated (1) | INUM (8) | Name (variable len) |

The allocated byte tells us if the entry is re-usable to store another file name.
If the value is 111 (true), then an active file holds this entry.
If the value is 000 (false), then it is reusable.

@param dir_inode: Pointer to the directory (parent) inode.
@param child_inum: Inode number of the file being added as an entry.
@param file_name: Name of the file being added.

@return true or false
*/
bool add_directory_entry(struct inode* dir_inode, ssize_t child_inum, char* file_name)
{
    // Check if dir_inode is actually a directory
    if(!S_ISDIR(dir_inode->i_mode)){
        fuse_log(FUSE_LOG_ERR, "%s : The parent inode is not a directory.\n", ADD_DIRECTORY_ENTRY);
        return false;
    }

    // Check for maximum lenth of file name
    ssize_t file_name_len = strlen(file_name);
    if(file_name_len > MAX_FILE_NAME_LENGTH){
        fuse_log(FUSE_LOG_ERR, "%s : File name is > 255 bytes.\n", ADD_DIRECTORY_ENTRY);
        return false;
    }

    unsigned short short_name_length = file_name_len;
    ssize_t new_entry_size = RECORD_FIXED_LEN + short_name_length;

    // If the directory inode has datablocks allocated, try to find space in them to add the entry.
    if(dir_inode->i_blocks_num > 0){
        for(ssize_t l_block_num = 0; l_block_num < dir_inode->i_blocks_num; l_block_num++)
        {
            ssize_t p_block_num = get_data_block_from_file_block(dir_inode, l_block_num);

            if(p_block_num <= 0)
            {
                fuse_log(FUSE_LOG_ERR, "%s : Failed to fetch physical data block number corresponfing to file's logical block number.\n", ADD_DIRECTORY_ENTRY);
                return false;
            }

            char* dblock = read_data_block(p_block_num);

            ssize_t curr_pos = 0;
            bool add_record = true;
            while(curr_pos <= LAST_POSSIBLE_RECORD)
            {
                unsigned short record_len = ((unsigned short*)(dblock + curr_pos))[0];
                bool allocated = ((bool*)(dblock + curr_pos + RECORD_LENGTH))[0];

                // If the record is allocated, new record cannot be added here. Note: while reading records, check for record length first to know where to stop.
                if(allocated)
                {
                    add_record = false;
                } else
                {
                    // Indicates a record has never been allocated at (and from) this position
                    if(record_len == 0)
                    {
                        // Check if there is enough space left in the data block
                        ssize_t rem_block_space = BLOCK_SIZE - curr_pos - RECORD_FIXED_LEN;
                        if(rem_block_space < file_name_len)
                            add_record = false;
                    } else
                    {
                        // Check for avaiable name length in the existing record (un-allcoated)
                        unsigned short avail_name_len = record_len - RECORD_FIXED_LEN;
                        // Available name length is smaller than required
                        if(avail_name_len < short_name_length)
                            add_record = false;
                    }
                }

                if(add_record)
                {
                    fuse_log(FUSE_LOG_DEBUG, "%s : Space found in data block %ld (physical block #%ld) of directory for entry.\n", ADD_DIRECTORY_ENTRY, l_block_num, p_block_num);
                    unsigned short record_length = RECORD_FIXED_LEN + short_name_length;
                    char* record = dblock + curr_pos;
                    // Add record length
                    ((unsigned short*)record)[0] = record_length;
                    // Add allocated byte
                    ((bool*)(record + RECORD_LENGTH))[0] = true;
                    // Add child INUM
                    ((ssize_t*)(record + RECORD_LENGTH + RECORD_ALLOCATED))[0] = child_inum;
                    // Add file name
                    // TODO: Make sure that when a file record is removed, the name is made all 0's
                    strncpy((char*)(record + RECORD_FIXED_LEN), file_name, file_name_len);

                    if(!write_data_block(p_block_num, dblock)){
                        fuse_log(FUSE_LOG_ERR, "%s : Error writing data block %ld to disk.\n", ADD_DIRECTORY_ENTRY, p_block_num);
                        altfs_free_memory(dblock);
                        return false;
                    }

                    altfs_free_memory(dblock);
                    return true;
                }

                curr_pos += record_len;
                add_record = true;
            }

            altfs_free_memory(dblock);
        }
    }
    fuse_log(FUSE_LOG_DEBUG, "%s : No space found in existing data blocks for directory entry, allocating a new block.\n", ADD_DIRECTORY_ENTRY);

    // Add a data block to the inode
    ssize_t data_block_num = allocate_data_block();
    if(data_block_num <= 0){
        fuse_log(FUSE_LOG_ERR, "%s : Error allocating new data block for directory entry.\n", ADD_DIRECTORY_ENTRY);
        return false;
    }

    char data_block[BLOCK_SIZE];
    memset(data_block, 0, BLOCK_SIZE);

    unsigned short record_length = RECORD_FIXED_LEN + short_name_length;
    // Add record length
    ((unsigned short*)data_block)[0] = record_length;
    // Add allocated byte
    ((bool*)(data_block + RECORD_LENGTH))[0] = true;
    // Add child INUM
    ((ssize_t*)(data_block + RECORD_LENGTH + RECORD_ALLOCATED))[0] = child_inum;
    // Add file name
    strncpy((char*)(data_block + RECORD_FIXED_LEN), file_name, file_name_len);

    if(!write_data_block(data_block_num, data_block)){
        fuse_log(FUSE_LOG_ERR, "%s : Error writing data block %ld to disk.\n", ADD_DIRECTORY_ENTRY, data_block_num);
        return false;
    }
    // TODO: Use the correct function when implemented
    if(!add_dblock_to_inode(dir_inode, data_block_num)){
        printf("couldn't add dblock to inode\n");
        return false;
    }
    
    dir_inode->i_file_size += BLOCK_SIZE;   // TODO: Should this be in the add datablock to inode function?
    return true;
}

// bool add_directory_entry(struct inode *root_inode, ssize_t child_inum, char* file_name)
// {
//     if(!S_ISDIR(root_inode->i_mode)){
//         fuse_log(FUSE_LOG_ERR, "%s : The root inode is not a directory. Exiting\n", ADD_DIRECTORY_ENTRY);
//         return false;
//     }

//     ssize_t file_name_len = strlen(file_name);
//     if(file_name_len > MAX_FILE_NAME_LENGTH){
//         fuse_log(FUSE_LOG_ERR, "%s : File name is > 255. Exiting\n", ADD_DIRECTORY_ENTRY);
//         return false;
//     }

//     unsigned short short_name_length = file_name_len; 
//     // 8 + 8 + 2 + file_name_len
//     ssize_t new_entry_size = INODE_SIZE + ADDRESS_PTR_SIZE + FILE_NAME_SIZE + short_name_length;
//     //if there is already a datablock for the dir file, we will add entry to it if it has space
//     if(root_inode->i_blocks_num > 0){
//         ssize_t data_block_num = get_data_block_from_file_block(root_inode, root_inode->i_blocks_num - 1);
        
//         if(data_block_num <= 0)
//         {
//             return false;
//         }

//         char* dblock = read_data_block(data_block_num);
//         ssize_t curr_pos = 0;
//         while(curr_pos < BLOCK_SIZE){
//             /*
//             INUM(8) | RECORD_LEN/ADDR_PTR(8 - either record len / space left) | FILE_STR_LEN | FILE_NAME
//             addr_ptr for earlier records holds the len/size of record.
//             However, for the last record it holds the BLOCKSIZE-(sum of all prev records len)
//             this is fetched in next_entry. 
//             For example, when the 1st entry is made to the block, the add_ptr holds the block size 4096
//             when a 2nd entry is made, the add_ptr of the 1st entry holds rec_len while the add_ptr of 2nd entry hols 4096-(sum of prev record len)
//             Hence, if BLOCK is able to accomodate more entries, it will be indicated in the add_ptr field of last entry.
//             */      
//             ssize_t next_entry_offset_from_curr = ((ssize_t*)(dblock+curr_pos+INODE_SIZE))[0]; // this gives record_len or space_left
//             unsigned short curr_entry_name_length = ((unsigned short* )(dblock+curr_pos + INODE_SIZE + ADDRESS_PTR_SIZE))[0];//len of filename/iname pointed by curr_pos
//             ssize_t curr_ptr_entry_length = INODE_SIZE + ADDRESS_PTR_SIZE + FILE_NAME_SIZE + curr_entry_name_length;
//             // the following condition holds true only for the last entry of the dir and if the block is able to accomodate new entry. Until the we move the curr_pos
//             if(next_entry_offset_from_curr - curr_ptr_entry_length >= new_entry_size){
//                 // updating curr entry addr with size of entry
//                 ((ssize_t* )(dblock+curr_pos+INODE_SIZE))[0] = curr_ptr_entry_length;
//                 if(curr_ptr_entry_length <= 0){
//                     return false;
//                 }
//                 ssize_t addr_ptr = next_entry_offset_from_curr - curr_ptr_entry_length; // this is essentially 4096-(sum of all prev record len)
//                 // This will even handle the padding.
//                 curr_pos += curr_ptr_entry_length;
//                 // add new entry
//                 ((ssize_t*) (dblock+curr_pos))[0] = child_inum;
//                 ((ssize_t*) (dblock+curr_pos+INODE_SIZE))[0] = addr_ptr;
//                 ((unsigned short*) (dblock+curr_pos+INODE_SIZE+ADDRESS_PTR_SIZE))[0] = short_name_length;
//                 strncpy((char*) (dblock+curr_pos+INODE_SIZE+ADDRESS_PTR_SIZE+FILE_NAME_SIZE), file_name, file_name_len);
//                 if(!write_dblock(data_block_num, dblock)){
//                     printf("Writing new entry of dir to dblock failed\n");
//                     return false;
//                 }
//                 return true;
//             }
//             if(next_entry_offset_from_curr <= 0){ // this will never be true unless there is a bug
//                 printf("Could be a bug, check this\n");
//                 return false;
//             }
//             // this will come here if diff is 0 or we can't accomodate in the current block
//             curr_pos += next_entry_offset_from_curr;
//         }
//         altfs_free_memory(dblock);
//     }
//     ssize_t data_block_num = create_new_dblock();
//     if(data_block_num<=0){
//         return false;
//     }
//     char dblock[BLOCK_SIZE];
//     memset(dblock, 0, BLOCK_SIZE);
//     ((ssize_t*) dblock)[0] = child_inum;
//     ssize_t addr_ptr = BLOCK_SIZE;
//     ((ssize_t*) (dblock+INODE_SIZE))[0] = addr_ptr;
//     ((unsigned short*) (dblock + INODE_SIZE + ADDRESS_PTR_SIZE))[0] = short_name_length;
//     strncpy((char*)(dblock+INODE_SIZE+ADDRESS_PTR_SIZE+FILE_NAME_SIZE), file_name, file_name_len);
//     if(!write_dblock(data_block_num, dblock)){
//         printf("Could be a bug, check this\n");
//         return false;
//     }
//     //TODO: Verify below check and code
//     if(!add_dblock_to_inode(root_inode, data_block_num)){
//         printf("couldn't add dblock to inode\n");
//         return false;
//     }
    
//     root_inode->file_size += BLOCK_SIZE;
//     return true;
// }

bool initialize_fs()
{
    if(!altfs_makefs()){
        fuse_log(FUSE_LOG_ERR, "%s : Makefs failed\n", INITIALIZE_FS);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s : Successfully ran makefs\n", INITIALIZE_FS);
    
    struct inode* root_dir = read_inode(ROOT_INODE_NUM);
    if(root_dir == NULL){
        fuse_log(FUSE_LOG_ERR, "%s : The root inode is null\n", INITIALIZE_FS);
        return false;
    }

    // TODO: If root node is already created and we are remounting the FS, 
    // new entries need not be created again and just verify it using allocated = true
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

    char* dir_name = ".";
    if(!add_directory_entry(root_dir, ROOT_INODE_NUM, dir_name)){
        fuse_log(FUSE_LOG_ERR, "%s Failed to add . entry for root directory\n", INITIALIZE_FS);
        return false;
    }

    fuse_log(FUSE_LOG_DEBUG, "%s Successfully added . entry for root directory\n", INITIALIZE_FS);

    dir_name = "..";
    if(!add_directory_entry(root_dir, ROOT_INODE_NUM, dir_name)){
        fuse_log(FUSE_LOG_ERR, "%s Failed to add .. entry for root directory\n", INITIALIZE_FS);
        return false;
    }

    fuse_log(FUSE_LOG_DEBUG, "%s Successfully added .. entry for root directory\n", INITIALIZE_FS);

    if(!write_inode(ROOT_INODE_NUM, root_dir)){
        fuse_log(FUSE_LOG_ERR, "%s Failed to write inode entry for root directory\n", INITIALIZE_FS);
        return false;
    }
    
    fuse_log(FUSE_LOG_DEBUG, "%s Successfully wrote root dir inode with %ld data blocks\n", INITIALIZE_FS, root->i_blocks_num);
    
    // free memory on disk/ in memory for root since it has been written to disk/ memory
    if (root_dir != NULL)
    {
        free(root_dir);
        fuse_log(FUSE_LOG_DEBUG, "%s Successfully freed memory space allocated to root entry\n", INITIALIZE_FS);
    }
    // Create a cache that can be used to implement namei
    create_inode_cache(&inodeCache, CACHE_CAPACITY);
    
    fuse_log(FUSE_LOG_DEBUG, "%s Successfully created inode cache to retrieve inode data faster\n", INITIALIZE_FS);
    return true;
}
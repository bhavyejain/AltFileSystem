#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<stdio.h>
#include<fuse.h>
#include "../header/superblock_layer.h"
#include "../header/disk_layer.h"
#include "../header/inode_ops.h"
#include "../header/data_block_ops.h"
#include "../header/initialize_fs_ops.h"
#include "../header/inode_cache.h"

static struct inode_cache inodeCache;

// Get data block number for given file block
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
    }

    // If file block num >= 12 and < 512 => single indirect block
    else if(file_block_num < NUM_OF_DIRECT_BLOCKS + NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)
    {
        if(file_inode->i_single_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR,"%s : Single indirect block is set to 0 for inode. Exiting\n", GET_DBLOCK_FROM_FBLOCK);
            return data_block_num;
        }

        // Read single indirect block and extract data block num from file block num
        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(file_inode->i_single_indirect);
        data_block_num = single_indirect_block_arr[file_block_num - NUM_OF_DIRECT_BLOCKS];
        altfs_free_memory(single_indirect_block_arr);

        fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from single indirect block\n", GET_DBLOCK_FROM_FBLOCK, data_block_num);
    }
    
    // If file block num >= 512 and < 512*512 => double indirect block
    else if(file_block_num < NUM_OF_DIRECT_BLOCKS + NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR + NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR)
    {
        if(file_inode->i_double_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR,"%s : Double indirect block is set to 0 for inode. Exiting\n", GET_DBLOCK_FROM_FBLOCK);
            return data_block_num;
        }

        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(file_inode->i_double_indirect);
        ssize_t offset = file_block_num - NUM_OF_DIRECT_BLOCKS - NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR;
        data_block_num = double_indirect_block_arr[offset / NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR];
        altfs_free_memory(double_indirect_block_arr);

        if(data_block_num <= 0){
            fuse_log(FUSE_LOG_ERR, "%s : Double indirect block num <= 0. Exiting\n", GET_DBLOCK_FROM_FBLOCK);
            return -1;
        }

        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);
        data_block_num = single_indirect_block_arr[offset % NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR];
        altfs_free_memory(single_indirect_block_arr);

        fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from double indirect block\n", GET_DBLOCK_FROM_FBLOCK, data_block_num);
    }
    // If file block num >= 512*512 and < 512*512*512 => triple indirect block
    else
    {
        if(file_inode->i_triple_indirect == 0)
        {
            fuse_log(FUSE_LOG_ERR,"%s : Triple indirect block is set to 0 for inode. Exiting\n", GET_DBLOCK_FROM_FBLOCK);
            return data_block_num;
        }

        ssize_t* triple_indirect_block_arr = (ssize_t*) read_data_block(file_inode->i_triple_indirect);
        ssize_t offset = file_block_num - NUM_OF_DIRECT_BLOCKS - NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR - NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR;
        data_block_num = triple_indirect_block_arr[offset / NUM_OF_DOUBLE_INDIRECT_BLOCK_ADDR];
        altfs_free_memory(triple_indirect_block_arr);

        if(data_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Triple indirect block num <= 0. Exiting\n", GET_DBLOCK_FROM_FBLOCK);
            return -1;
        }
        
        ssize_t* double_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);
        data_block_num = double_indirect_block_arr[(offset/NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR)%NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR];
        altfs_free_memory(double_indirect_block_arr);
        
        if(data_block_num<=0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Double indirect block num <= 0. Exiting\n", GET_DBLOCK_FROM_FBLOCK);
            return -1;
        }

        ssize_t* single_indirect_block_arr = (ssize_t*) read_data_block(data_block_num);
        data_block_num = single_indirect_block_arr[offset%NUM_OF_SINGLE_INDIRECT_BLOCK_ADDR];
        altfs_free_memory(single_indirect_block_arr);

        fuse_log(FUSE_LOG_DEBUG, "%s : Returning data block num %ld from triple indirect block\n", GET_DBLOCK_FROM_FBLOCK, data_block_num);
    }
    return data_block_num;
}

// TODO: Check the logic properly and make changes to constants
bool altfs_add_inode_entry(struct inode *root_inode, ssize_t inode_num, char* inode_name)
{
    if(!S_ISDIR(root_inode->i_mode)){
        fuse_log(FUSE_LOG_ERR, "%s : The root inode is not a directory. Exiting\n", ALTFS_ADD_INODE_ENTRY);
        return false;
    }

    ssize_t file_name_len = strlen(inode_name);
    if(file_name_len > MAX_FILE_NAME_LENGTH){
        fuse_log(FUSE_LOG_ERR, "%s : File name is > 255. Exiting\n", ALTFS_ADD_INODE_ENTRY);
        return false;
    }

    unsigned short short_name_length = file_name_len; 
    // 8 + 8 + 2 + file_name_len
    ssize_t new_entry_size = INODE_SIZE + ADDRESS_PTR_SIZE + FILE_NAME_SIZE + short_name_length;
    //if there is already a datablock for the dir file, we will add entry to it if it has space
    if(root_inode->i_blocks_num > 0){
        ssize_t data_block_num = get_data_block_from_file_block(root_inode, root_inode->i_blocks_num - 1);
        
        if(data_block_num <= 0)
        {
            return false;
        }

        char* dblock = read_data_block(data_block_num);
        ssize_t curr_pos = 0;
        while(curr_pos < BLOCK_SIZE){
            /*
            INUM(8) | RECORD_LEN/ADDR_PTR(8 - either record len / space left) | FILE_STR_LEN | FILE_NAME
            addr_ptr for earlier records holds the len/size of record.
            However, for the last record it holds the BLOCKSIZE-(sum of all prev records len)
            this is fetched in next_entry. 
            For example, when the 1st entry is made to the block, the add_ptr holds the block size 4096
            when a 2nd entry is made, the add_ptr of the 1st entry holds rec_len while the add_ptr of 2nd entry hols 4096-(sum of prev record len)
            Hence, if BLOCK is able to accomodate more entries, it will be indicated in the add_ptr field of last entry.
            */      
            ssize_t next_entry_offset_from_curr = ((ssize_t*)(dblock+curr_pos+INODE_SIZE))[0]; // this gives record_len or space_left
            unsigned short curr_entry_name_length = ((unsigned short* )(dblock+curr_pos + INODE_SIZE + ADDRESS_PTR_SIZE))[0];//len of filename/iname pointed by curr_pos
            ssize_t curr_ptr_entry_length = INODE_SIZE + ADDRESS_PTR_SIZE + FILE_NAME_SIZE + curr_entry_name_length;
            // the following condition holds true only for the last entry of the dir and if the block is able to accomodate new entry. Until the we move the curr_pos
            if(next_entry_offset_from_curr - curr_ptr_entry_length >= new_entry_size){
                // updating curr entry addr with size of entry
                ((ssize_t* )(dblock+curr_pos+INODE_SIZE))[0] = curr_ptr_entry_length;
                if(curr_ptr_entry_length <= 0){
                    return false;
                }
                ssize_t addr_ptr = next_entry_offset_from_curr - curr_ptr_entry_length; // this is essentially 4096-(sum of all prev record len)
                // This will even handle the padding.
                curr_pos += curr_ptr_entry_length;
                // add new entry
                ((ssize_t*) (dblock+curr_pos))[0] = child_inode_num;
                ((ssize_t*) (dblock+curr_pos+INODE_SIZE))[0] = addr_ptr;
                ((unsigned short*) (dblock+curr_pos+INODE_SIZE+ADDRESS_PTR_SIZE))[0] = short_name_length;
                strncpy((char*) (dblock+curr_pos+INODE_SIZE+ADDRESS_PTR_SIZE+FILE_NAME_SIZE), child_name, file_name_len);
                if(!write_dblock(data_block_num, dblock)){
                    printf("Writing new entry of dir to dblock failed\n");
                    return false;
                }
                return true;
            }
            if(next_entry_offset_from_curr <= 0){ // this will never be true unless there is a bug
                printf("Could be a bug, check this\n");
                return false;
            }
            // this will come here if diff is 0 or we can't accomodate in the current block
            curr_pos += next_entry_offset_from_curr;
        }
        altfs_free_memory(dblock);
    }
    ssize_t data_block_num = create_new_dblock();
    if(data_block_num<=0){
        return false;
    }
    char dblock[BLOCK_SIZE];
    memset(dblock, 0, BLOCK_SIZE);
    ((ssize_t*) dblock)[0] = child_inode_num;
    ssize_t addr_ptr = BLOCK_SIZE;
    ((ssize_t*) (dblock+INODE_SIZE))[0] = addr_ptr;
    ((unsigned short*) (dblock + INODE_SIZE + ADDRESS_PTR_SIZE))[0] = short_name_length;
    strncpy((char*)(dblock+INODE_SIZE+ADDRESS_PTR_SIZE+FILE_NAME_SIZE), child_name, name_len);
    if(!write_dblock(data_block_num, dblock)){
        printf("Could be a bug, check this\n");
        return false;
    }
    //TODO: Verify below check and code
    if(!add_dblock_to_inode(inode, data_block_num)){
        printf("couldn't add dblock to inode\n");
        return false;
    }
    
    file_inode->file_size += BLOCK_SIZE;
    return true;
}

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
    if(!altfs_add_inode_entry(root_dir, ROOT_INODE_NUM, dir_name)){
        fuse_log(FUSE_LOG_ERR, "%s Failed to add . entry for root directory\n", INITIALIZE_FS);
        return false;
    }

    fuse_log(FUSE_LOG_DEBUG, "%s Successfully added . entry for root directory\n", INITIALIZE_FS);

    dir_name = "..";
    if(!altfs_add_inode_entry(root_dir, ROOT_INODE_NUM, dir_name)){
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
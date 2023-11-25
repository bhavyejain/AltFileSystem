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
#include "../header/namei_ops.h"

struct fileposition get_file_position_in_dir(const char* const file_name, const struct inode* const parent_inode)
{
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

    for(ssize_t l_block_num = 0; l_block_num < parent_inode->i_blocks_num; l_block_num++)
    {
        filepos.p_plock_num = get_disk_block_from_inode_block(parent_inode, l_block_num);

        if(filepos.p_block_num <= 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Failed to fetch physical data block number corresponfing to file's logical block number.\n", GET_FILE_POS_IN_DIR);
            return filepos;
        }

        filepos.p_block = read_data_block(p_block_num);

        // traverse the data block to find an inode entry with the given file name
        ssize_t curr_pos = 0;
        while(curr_pos <= LAST_POSSIBLE_RECORD)
        {
            unsigned short record_len = ((unsigned short*)(filepos.p_block + curr_pos))[0];
            bool allocated = ((bool*)(filepos.p_block + curr_pos + RECORD_LENGTH))[0];
            char *curr_file_name = ((char*)(filepos.p_block + curr_pos + RECORD_FIXED_LEN))[0];
            unsigned short curr_file_name_len = ((unsigned short)(record_len - RECORD_FIXED_LEN));

            if (allocated && 
                curr_file_name_len == strlen(file_name) && 
                strncmp(curr_file_name, file_name, curr_file_name_len) == 0) {
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
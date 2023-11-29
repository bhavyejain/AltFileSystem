#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"
#include "../src/inode_ops.c"
#include "../src/data_block_ops.c"
#include "../src/inode_data_block_ops.c"
#include "../src/inode_cache.c"
#include "../src/filesystem_ops.c"

#define FILESYSTEM_OPS_TEST "filesystem_ops_test"

void print_dir_contents(struct inode** node, ssize_t iblock)
{
    printf("======= DIR CONTENTS ======\n");
    ssize_t prev = 0;
    ssize_t end = (*node)->i_blocks_num - 1;
    if (iblock == -1)
    {
        iblock = 0;
    } else
    {
        if(iblock > end)
        {
            printf("*** invalid iblock to print directory contents, max: %ld ***\n", (*node)->i_blocks_num);
            return;
        }
        end = iblock;
    }

    while(iblock <= end)
    {
        ssize_t dblock = get_disk_block_from_inode_block((*node), iblock, &prev);
        char* buff = read_data_block(dblock);
        for(ssize_t i = 0; i < BLOCK_SIZE; i++)
        {
            char ch = buff[i];
            if(ch == '\0')
                printf(" ");
            else if((ch >= 'a' && ch <= 'z') || (ch == '_') || (ch >= '0' && ch <= '9'))
                printf("%c", ch);
        }
        iblock++;
    }

    printf("\n==========================\n");
    printf("\n");
}

bool test_add_directory_entry()
{
    printf("\n%s : Testing add directory entry...\n", FILESYSTEM_OPS_TEST);
    ssize_t inum1 = allocate_inode();
    struct inode* node = get_inode(inum1);
    node->i_mode = S_IFDIR;
    char* dir_name = "directory1";
    if(!add_directory_entry(&node, 123, dir_name))
    {
        fprintf(stderr, "%s : Failed to add directory entry: %s\n", FILESYSTEM_OPS_TEST, dir_name);
        altfs_free_memory(node);
        return false;
    }
    if(node->i_blocks_num == 0)
    {
        fprintf(stderr, "%s : Block not added for directory entry: %s\n", FILESYSTEM_OPS_TEST, dir_name);
        altfs_free_memory(node);
        return false;
    }
    print_dir_contents(&node, 0);
    // fill till populate next block
    printf("%s : Adding many directory entries...\n", FILESYSTEM_OPS_TEST);
    char name[50];
    for(int i = 0; i < 149; i++)
    {
        snprintf(name, sizeof(name), "this_is_an_excruciatingly_long_directory_%d", i);
        if(!add_directory_entry(&node, i, name))
        {
            fprintf(stderr, "%s : Failed to add directory entry: %s\n", FILESYSTEM_OPS_TEST, dir_name);
            altfs_free_memory(node);
            return false;
        }
    }
    if(node->i_blocks_num == 1)
    {
        fprintf(stderr, "%s : Block not added for directory entry: %s\n", FILESYSTEM_OPS_TEST, dir_name);
        altfs_free_memory(node);
        return false;
    }
    printf("%s : Total inode blocks after adding entries: %ld\n", FILESYSTEM_OPS_TEST, node->i_blocks_num);
    // printf("\n----- First directory entry block -----\n");
    // print_dir_contents(&node, 0);
    // printf("\n----- Second directory entry block -----\n");
    // print_dir_contents(&node, 1);
    
    write_inode(inum1, node);
    
    // delete some entries from the first block
    char* buff = read_data_block(node->i_direct_blocks[0]);
    ssize_t offset = 0;
    for(int i = 0; i < 5; i++)
    {
        unsigned short rec_len = ((unsigned short *)(buff + offset))[0];
        offset += rec_len;
    }
    char new_buff[BLOCK_SIZE];
    memset(new_buff, 0, BLOCK_SIZE);
    memcpy(new_buff, buff + offset, (BLOCK_SIZE - offset));
    write_data_block(node->i_direct_blocks[0], new_buff);
    // printf("\n----- First directory entry block -----\n");
    // print_dir_contents(&node, 0);

    // try adding another entry
    dir_name = "directory_hahaha";
    if(!add_directory_entry(&node, 123, dir_name))
    {
        fprintf(stderr, "%s : Failed to add directory entry: %s\n", FILESYSTEM_OPS_TEST, dir_name);
        altfs_free_memory(node);
        return false;
    }
    if(node->i_blocks_num > 2)
    {
        fprintf(stderr, "%s : Entry added at wrong place: %s\n", FILESYSTEM_OPS_TEST, dir_name);
        altfs_free_memory(node);
        return false;
    }
    // printf("\n----- First directory entry block -----\n");
    // print_dir_contents(&node, 0);
    
    printf("\n%s : Ran all tests for add directory entry!!!\n", FILESYSTEM_OPS_TEST);
    free_inode(inum1);
    altfs_free_memory(node);
    return true;
}

bool test_get_file_position()
{
    printf("\n%s : Testing get file position...\n", FILESYSTEM_OPS_TEST);
    ssize_t inum1 = allocate_inode();
    struct inode* node = get_inode(inum1);
    node->i_mode = S_IFDIR;
    char* dir_name = "directory1";
    add_directory_entry(&node, 111, dir_name);
    dir_name = "directory2";
    add_directory_entry(&node, 222, dir_name);
    dir_name = "directory3";
    add_directory_entry(&node, 333, dir_name);
    write_inode(inum1, node);

    struct fileposition fp = get_file_position_in_dir("directory2", node);
    if(fp.p_block == NULL)
    {
        fprintf(stderr, "%s : Could not fetch data block containing entry for file: %s\n", FILESYSTEM_OPS_TEST, "directory2");
        altfs_free_memory(node);
        return false;
    }
    if(fp.offset != 20)
    {
        fprintf(stderr, "%s : Incorrect offset for file: %s\n", FILESYSTEM_OPS_TEST, "directory2");
        altfs_free_memory(fp.p_block);
        altfs_free_memory(node);
        return false;
    }
    if(fp.p_plock_num != node->i_direct_blocks[0])
    {
        fprintf(stderr, "%s : Incorrect disk block number for block containing file: %s\n", FILESYSTEM_OPS_TEST, "directory2");
        altfs_free_memory(fp.p_block);
        altfs_free_memory(node);
        return false;
    }

    printf("\n%s : Ran all tests for get file position!!!\n", FILESYSTEM_OPS_TEST);
    free_inode(inum1);
    altfs_free_memory(node);
    return true;
}

bool test_initialize_fs()
{
    printf("%s : Testing filesystem initialization...\n", FILESYSTEM_OPS_TEST);
    if(!initialize_fs())
    {
        fprintf(stderr, "%s : Filesystem initialization failed!\n", FILESYSTEM_OPS_TEST);
        return false;
    }
    printf("%s : Filesystem initialized successfully!\n", FILESYSTEM_OPS_TEST);
    return true;
}

int main()
{
    printf("=============== TESTING DIRECTORY & INIT FS OPERATIONS =============\n\n");
    // Create filesystem (assumes superblock layer tests pass)
    if(!altfs_makefs())
    {
        printf("Altfs makefs failed!\n");
        return -1;
    }
    printf("Makefs complete!\n");

    if(!test_add_directory_entry())
    {
        printf("%s : Testing adding directory entries failed!\n", FILESYSTEM_OPS_TEST);
        return -1;
    }

    if(!test_get_file_position())
    {
        printf("%s : Testing get file position failed!\n", FILESYSTEM_OPS_TEST);
        return -1;
    }

    printf("=============== ALL TESTS RUN =============\n\n");
    return 0;
}
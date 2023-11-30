#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../src/disk_layer.c"
#include "../src/superblock_layer.c"
#include "../src/inode_ops.c"
#include "../src/data_block_ops.c"
#include "../src/inode_data_block_ops.c"
#include "../src/inode_cache.c"
#include "../src/directory_ops.c"
#include "../src/interface_layer.c"

#include "test_helpers.c"

#define INTERFACE_LAYER_TEST "test_interface_layer"

bool test_getattr()
{
    printf("\n----- %s : Testing getattr() -----\n", INTERFACE_LAYER_TEST);
    ssize_t dir_inum = allocate_inode();
    struct inode* root = get_inode(ROOT_INODE_NUM);
    add_directory_entry(&root, dir_inum, "dir1");
    write_inode(ROOT_INODE_NUM, root);

    ssize_t file_inum = allocate_inode();
    struct inode* dir1 = get_inode(dir_inum);
    dir1->i_mode = S_IFDIR | DEFAULT_PERMISSIONS;
    add_directory_entry(&dir1, file_inum, "file1");
    write_inode(dir_inum, dir1);

    struct inode* file1 = get_inode(file_inum);
    printf("\n");

    struct stat* st = (struct stat*)calloc(1, sizeof(struct stat));
    if(altfs_getattr("/", &st) != 0)
    {
        fprintf(stderr, "%s : Failed to get attributes for /.\n", INTERFACE_LAYER_TEST);
        altfs_free_memory(root);
        altfs_free_memory(dir1);
        altfs_free_memory(file1);
        altfs_free_memory(st);
        return false;
    }
    if(st->st_ino != ROOT_INODE_NUM || st->st_blocks != root->i_blocks_num || st->st_size != BLOCK_SIZE)
    {
        fprintf(stderr, "%s : Wrong values stored in st for /. Should be (%ld, %ld, %ld), are (%ld, %ld, %ld).\n", INTERFACE_LAYER_TEST,
            ROOT_INODE_NUM, root->i_blocks_num, BLOCK_SIZE, st->st_ino, st->st_blocks, st->st_size);
        altfs_free_memory(root);
        altfs_free_memory(dir1);
        altfs_free_memory(file1);
        altfs_free_memory(st);
        return false;
    }
    altfs_free_memory(root);
    printf("\n");

    if(altfs_getattr("/dir1", &st) != 0)
    {
        fprintf(stderr, "%s : Failed to get attributes for /dir1.\n", INTERFACE_LAYER_TEST);
        altfs_free_memory(dir1);
        altfs_free_memory(file1);
        altfs_free_memory(st);
        return false;
    }
    if(st->st_ino != dir_inum || st->st_blocks != dir1->i_blocks_num || st->st_size != BLOCK_SIZE)
    {
        fprintf(stderr, "%s : Wrong values stored in st for /dir1. Should be (%ld, %ld, %ld), are (%ld, %ld, %ld).\n", INTERFACE_LAYER_TEST,
            dir_inum, dir1->i_blocks_num, BLOCK_SIZE, st->st_ino, st->st_blocks, st->st_size);
        altfs_free_memory(dir1);
        altfs_free_memory(file1);
        altfs_free_memory(st);
        return false;
    }
    altfs_free_memory(dir1);
    printf("\n");

    if(altfs_getattr("/dir1/file1", &st) != 0)
    {
        fprintf(stderr, "%s : Failed to get attributes for /dir1/file1.\n", INTERFACE_LAYER_TEST);
        altfs_free_memory(file1);
        altfs_free_memory(st);
        return false;
    }
    if(st->st_ino != file_inum || st->st_blocks != file1->i_blocks_num || st->st_size != 0)
    {
        fprintf(stderr, "%s : Wrong values stored in st for /dir1/file1. Should be (%ld, %ld, %d), are (%ld, %ld, %ld).\n", INTERFACE_LAYER_TEST,
            file_inum, file1->i_blocks_num, 0, st->st_ino, st->st_blocks, st->st_size);
        altfs_free_memory(file1);
        altfs_free_memory(st);
        return false;
    }
    altfs_free_memory(file1);
    printf("\n");

    if(altfs_getattr("/file2", &st) != -ENOENT)
    {
        fprintf(stderr, "%s : Should have failed for /file2 but returned passed.\n", INTERFACE_LAYER_TEST);
        altfs_free_memory(st);
        return false;
    }
    altfs_free_memory(st);

    printf("----- %s : Tested getattr()! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

int main()
{
    printf("=============== TESTING INTERFACE OPERATIONS =============\n\n");

    if(!altfs_makefs())
    {
        printf("Altfs makefs failed!\n");
        return -1;
    }
    printf("Makefs complete!\n");

    if(!altfs_init())
    {
        fprintf(stderr, "Filesystem initialization failed!\n");
        return -1;
    }
    printf("Filesystem initialized successfully!\n");

    if(!test_getattr())
    {
        printf("%s : Testing altfs_getattr() failed!\n", INTERFACE_LAYER_TEST);
        return -1;
    }

    printf("=============== ALL TESTS RUN =============\n\n");
    return 0;
}

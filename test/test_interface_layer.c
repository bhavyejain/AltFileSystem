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

    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_access()
{
    printf("\n----- %s : Testing access() -----\n", INTERFACE_LAYER_TEST);
    
    if(altfs_access("/") != 0)
    {
        fprintf(stderr, "%s : Failed to access /.", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    if(altfs_access("/dir1/file1") != 0)
    {
        fprintf(stderr, "%s : Failed to access /dir1/file1.", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    if(altfs_access("/dir1/file2") != -ENOENT)
    {
        fprintf(stderr, "%s : Got wrong access to /dir1/file2", INTERFACE_LAYER_TEST);
        return false;
    }

    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_mkdir()
{
    printf("\n----- %s : Testing mkdir() -----\n", INTERFACE_LAYER_TEST);
    
    // Already exists
    if(altfs_mkdir("/dir1", DEFAULT_PERMISSIONS))
    {
        fprintf(stderr, "%s : Directory already existed, but did not fail.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Parent does not exist
    if(altfs_mkdir("/dir2/dir1", DEFAULT_PERMISSIONS))
    {
        fprintf(stderr, "%s : Parent does not exist, but did not fail.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Should succeed
    if(!altfs_mkdir("/dir2", DEFAULT_PERMISSIONS))
    {
        fprintf(stderr, "%s : Failed to create directory /dir2.\n", INTERFACE_LAYER_TEST);
        return false;
    }

    ssize_t dir2_inum = name_i("/dir2");
    struct inode* dir2 = get_inode(dir2_inum);
    if(dir2->i_blocks_num != 1)
    {
        fprintf(stderr, "%s : Incorrect number of blocks in directory /dir2: %ld\n", INTERFACE_LAYER_TEST, dir2->i_blocks_num);
        altfs_free_memory(dir2);
        return false;
    }
    if(dir2->i_child_num != 2)
    {
        fprintf(stderr, "%s : Incorrect number of children in directory /dir2: %ld\n", INTERFACE_LAYER_TEST, dir2->i_child_num);
        altfs_free_memory(dir2);
        return false;
    }
    printf("\n");

    struct inode* root = get_inode(ROOT_INODE_NUM);
    if(root->i_child_num != 4)
    {
        fprintf(stderr, "%s : Incorrect number of children in parent directory /: %ld\n", INTERFACE_LAYER_TEST, root->i_child_num);
        altfs_free_memory(dir2);
        altfs_free_memory(root);
        return false;
    }
    altfs_free_memory(root);
    printf("\n");

    // Check for entry "."
    struct fileposition fp = get_file_position_in_dir(".", dir2);
    if(fp.offset == -1)
    {
        fprintf(stderr, "%s : Failed to locate . in directory /dir2.\n", INTERFACE_LAYER_TEST);
        altfs_free_memory(fp.p_block);
        altfs_free_memory(dir2);
        return false;
    }
    char* record = fp.p_block + fp.offset;
    ssize_t t1 = ((ssize_t*)(record + RECORD_LENGTH))[0];
    if(t1 != dir2_inum)
    {
        fprintf(stderr, "%s : Wrong inum for . in directory /dir2.\n", INTERFACE_LAYER_TEST);
        altfs_free_memory(fp.p_block);
        altfs_free_memory(dir2);
        return false;
    }
    altfs_free_memory(fp.p_block);
    printf("\n");

    // Check for entry ".."
    fp = get_file_position_in_dir("..", dir2);
    if(fp.offset == -1)
    {
        fprintf(stderr, "%s : Failed to locate .. in directory /dir2.\n", INTERFACE_LAYER_TEST);
        altfs_free_memory(fp.p_block);
        altfs_free_memory(dir2);
        return false;
    }
    record = fp.p_block + fp.offset;
    t1 = ((ssize_t*)(record + RECORD_LENGTH))[0];
    if(t1 != ROOT_INODE_NUM)
    {
        fprintf(stderr, "%s : Wrong inum for .. in directory /dir2.\n", INTERFACE_LAYER_TEST);
        altfs_free_memory(fp.p_block);
        altfs_free_memory(dir2);
        return false;
    }
    altfs_free_memory(fp.p_block);

    altfs_free_memory(dir2);
    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_mknod()
{
    printf("\n----- %s : Testing mknod() -----\n", INTERFACE_LAYER_TEST);
    
    if(!altfs_mknod("/dir2/file2", S_IFREG|S_IRUSR|S_IRGRP|S_IROTH, -1))
    {
        fprintf(stderr, "%s : Failed to create file /dir2/file2.\n", INTERFACE_LAYER_TEST);
        return false;
    }

    ssize_t inum = name_i("/dir2/file2");
    if(inum <= ROOT_INODE_NUM)
    {
        fprintf(stderr, "%s : Failed to get inum for file /dir2/file2.\n", INTERFACE_LAYER_TEST);
        return false;
    }

    struct inode* node = get_inode(inum);
    if(node->i_blocks_num != 0)
    {
        fprintf(stderr, "%s : New file /dir2/file2 has more than 0 blocks: %ld.\n", INTERFACE_LAYER_TEST, node->i_blocks_num);
        altfs_free_memory(node);
        return false;
    }
    if(node->i_file_size != 0)
    {
        fprintf(stderr, "%s : New file /dir2/file2 has non 0 size: %ld.\n", INTERFACE_LAYER_TEST, node->i_file_size);
        altfs_free_memory(node);
        return false;
    }

    altfs_free_memory(node);
    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_open()
{
    printf("\n----- %s : Testing open() -----\n", INTERFACE_LAYER_TEST);

    // Open non existing file
    if(altfs_open("/file3", O_RDONLY) != -ENOENT)
    {
        fprintf(stderr, "%s : Did not fail for non existing file /file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Open non existing file with O_CREAT
    ssize_t file3_inum = altfs_open("/dir2/file3", O_RDWR|O_CREAT);
    if(file3_inum < ROOT_INODE_NUM)
    {
        fprintf(stderr, "%s : Failed to create file /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    struct inode* file3 = get_inode(file3_inum);
    if(file3->i_blocks_num != 0 || file3->i_file_size != 0)
    {
        fprintf(stderr, "%s : File /dir2/file3 created incorrectly.\n", INTERFACE_LAYER_TEST);
        altfs_free_memory(file3);
        return false;
    }
    altfs_free_memory(file3);
    if(name_i("/dir2/file3") != file3_inum)
    {
        fprintf(stderr, "%s : Namei did not work correctly for file /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Open valid file with correct permissions
    if(altfs_open("/dir2/file3", O_RDWR) < ROOT_INODE_NUM)
    {
        fprintf(stderr, "%s : Failed to open file /dir1/file1.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Fail becuase of permissions
    if(altfs_open("/dir2/file2", O_WRONLY) != -EACCES)
    {
        fprintf(stderr, "%s : Permissions did not check correctly for /dir2/file2.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    
    // TODO: Add test for truncate
    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_write()
{
    printf("\n----- %s : Testing write() -----\n", INTERFACE_LAYER_TEST);

    // Tests assume the file was opened for writing before so the file exists.
    printf("TEST 1\n");
    char* data = "Hello world!";
    if(altfs_write("/dir2/file3", data, 0, 0) != 0)
    {
        fprintf(stderr, "%s : Did not return 0 when tried to write 0 bytes.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(altfs_write("/dir2/file3", data, 1, -1) != -EINVAL)
    {
        fprintf(stderr, "%s : Did not return error when tried to write at negative offset.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    ssize_t inum = name_i("/dir2/file3");

    // Will add new data blocks
    printf("TEST 2\n");
    if(altfs_write("/dir2/file3", data, 12, 0) != 12)
    {
        fprintf(stderr, "%s : Did not write full string to /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    struct inode* file = get_inode(inum);
    if(file->i_blocks_num != 1 || file->i_file_size != 12)
    {
        fprintf(stderr, "%s : File size for /dir2/file3 not correct. n_blocks: %ld, size (bytes): %ld\n", INTERFACE_LAYER_TEST, file->i_blocks_num, file->i_file_size);
        altfs_free_memory(file);
        return false;
    }
    char* buff = read_data_block(file->i_direct_blocks[0]);
    if(strncmp(data, buff, 12) != 0)
    {
        fprintf(stderr, "%s : Incorrect data written in /dir2/file3. Should be: |%s|, was: |%s|\n", INTERFACE_LAYER_TEST, data, buff);
        altfs_free_memory(file);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(file);
    altfs_free_memory(buff);
    printf("\n");

    // Will write in the same datablock again (add)
    printf("TEST 3\n");
    if(altfs_write("/dir2/file3", data, 12, 12) != 12)
    {
        fprintf(stderr, "%s : Did not write full string to /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    file = get_inode(inum);
    if(file->i_blocks_num != 1 || file->i_file_size != 24)
    {
        fprintf(stderr, "%s : File size for /dir2/file3 not correct. n_blocks: %ld, size (bytes): %ld\n", INTERFACE_LAYER_TEST, file->i_blocks_num, file->i_file_size);
        altfs_free_memory(file);
        return false;
    }
    buff = read_data_block(file->i_direct_blocks[0]);
    if(strncmp("Hello world!Hello world!", buff, 24) != 0)
    {
        fprintf(stderr, "%s : Incorrect data written in /dir2/file3. Should be: |Hello world!Hello world!|, was: |%s|\n", INTERFACE_LAYER_TEST, buff);
        altfs_free_memory(file);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(file);
    altfs_free_memory(buff);
    printf("\n");

    // Will write in the next datablock (add)
    printf("TEST 4\n");
    if(altfs_write("/dir2/file3", data, 12, 4096) != 12)
    {
        fprintf(stderr, "%s : Did not write full string to /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    file = get_inode(inum);
    if(file->i_blocks_num != 2 || file->i_file_size != 4108)
    {
        fprintf(stderr, "%s : File size for /dir2/file3 not correct. n_blocks: %ld, size (bytes): %ld\n", INTERFACE_LAYER_TEST, file->i_blocks_num, file->i_file_size);
        altfs_free_memory(file);
        return false;
    }
    buff = read_data_block(file->i_direct_blocks[1]);
    if(strncmp(data, buff, 12) != 0)
    {
        fprintf(stderr, "%s : Incorrect data written in block 1 /dir2/file3. Should be: |%s|, was: |%s|\n", INTERFACE_LAYER_TEST, data, buff);
        altfs_free_memory(file);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(file);
    altfs_free_memory(buff);
    printf("\n");

    // Will write in two datablocks (one existing, one new)
    printf("TEST 4\n");
    if(altfs_write("/dir2/file3", data, 12, 8190) != 12)
    {
        fprintf(stderr, "%s : Did not write full string to /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    file = get_inode(inum);
    if(file->i_blocks_num != 3 || file->i_file_size != 8204)
    {
        fprintf(stderr, "%s : File size for /dir2/file3 not correct. n_blocks: %ld, size (bytes): %ld\n", INTERFACE_LAYER_TEST, file->i_blocks_num, file->i_file_size);
        altfs_free_memory(file);
        return false;
    }
    buff = read_data_block(file->i_direct_blocks[1]);
    if(strncmp(data, buff + 4094, 2) != 0)
    {
        fprintf(stderr, "%s : Incorrect data written in block 1 /dir2/file3. Should be: |%.2s|, was: |%.2s|\n", INTERFACE_LAYER_TEST, data, buff);
        altfs_free_memory(file);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(buff);
    buff = read_data_block(file->i_direct_blocks[2]);
    if(strncmp(data + 2, buff, 10) != 0)
    {
        fprintf(stderr, "%s : Incorrect data written in block 2 /dir2/file3. Should be: |%s|, was: |%s|\n", INTERFACE_LAYER_TEST, data, buff);
        altfs_free_memory(file);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(file);
    altfs_free_memory(buff);
    printf("\n");

    // Will accross 3 datablocks (all existing)
    printf("TEST 5\n");
    char big_data[4106]; // 5 + 4096 + 5
    memset(big_data, 'a', 4116);
    if(altfs_write("/dir2/file3", big_data, 4116, 4091) != 4116)
    {
        fprintf(stderr, "%s : Did not write full string to /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    file = get_inode(inum);
    if(file->i_blocks_num != 3 || file->i_file_size != 8204)
    {
        fprintf(stderr, "%s : File size for /dir2/file3 not correct. n_blocks: %ld, size (bytes): %ld\n", INTERFACE_LAYER_TEST, file->i_blocks_num, file->i_file_size);
        altfs_free_memory(file);
        return false;
    }
    buff = read_data_block(file->i_direct_blocks[0]);
    if(strncmp("aaaaa", buff + 4091, 5) != 0)
    {
        fprintf(stderr, "%s : Incorrect data written in block 0 in /dir2/file3. Should be: |%.5s|, was: |%.5s|\n", INTERFACE_LAYER_TEST, "aaaaa", buff);
        altfs_free_memory(file);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(buff);
    buff = read_data_block(file->i_direct_blocks[1]);
    if(strncmp(big_data, buff, 4096) != 0)
    {
        fprintf(stderr, "%s : Incorrect data written in block 1 in /dir2/file3. Should be: all a's, was: |%s|\n", INTERFACE_LAYER_TEST, buff);
        altfs_free_memory(file);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(buff);
    buff = read_data_block(file->i_direct_blocks[2]);
    if(strncmp("aaaaa", buff, 5) != 0)
    {
        fprintf(stderr, "%s : Incorrect data written in block 2 in /dir2/file3. Should be: |%s|, was: |%.5s|\n", INTERFACE_LAYER_TEST, "aaaaa", buff);
        altfs_free_memory(file);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(file);
    altfs_free_memory(buff);
    printf("\n");
    
    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_read()
{
    printf("\n----- %s : Testing read() -----\n", INTERFACE_LAYER_TEST);

    
    
    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_truncate()
{
    printf("\n----- %s : Testing truncate() -----\n", INTERFACE_LAYER_TEST);

    
    
    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_rename()
{
    printf("\n----- %s : Testing rename() -----\n", INTERFACE_LAYER_TEST);

    
    
    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_unlink()
{
    printf("\n----- %s : Testing unlink() -----\n", INTERFACE_LAYER_TEST);

    
    
    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_chmod()
{
    printf("\n----- %s : Testing chmod() -----\n", INTERFACE_LAYER_TEST);

    
    
    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_permissions()
{
    printf("\n----- %s : Testing permissions -----\n", INTERFACE_LAYER_TEST);

    
    
    printf("----- %s : Done! -----\n", INTERFACE_LAYER_TEST);
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

    if(!test_access())
    {
        printf("%s : Testing altfs_access() failed!\n", INTERFACE_LAYER_TEST);
        return -1;
    }

    if(!test_mkdir())
    {
        printf("%s : Testing altfs_mkdir() failed!\n", INTERFACE_LAYER_TEST);
        return -1;
    }

    if(!test_mknod())
    {
        printf("%s : Testing altfs_mknod() failed!\n", INTERFACE_LAYER_TEST);
        return -1;
    }

    if(!test_open())
    {
        printf("%s : Testing altfs_open() failed!\n", INTERFACE_LAYER_TEST);
        return -1;
    }

    if(!test_write())
    {
        printf("%s : Testing altfs_write() failed!\n", INTERFACE_LAYER_TEST);
        return -1;
    }

    // if(!test_read())
    // {
    //     printf("%s : Testing altfs_read() failed!\n", INTERFACE_LAYER_TEST);
    //     return -1;
    // }

    // if(!test_truncate())
    // {
    //     printf("%s : Testing altfs_truncate() failed!\n", INTERFACE_LAYER_TEST);
    //     return -1;
    // }

    // if(!test_rename())
    // {
    //     printf("%s : Testing altfs_rename() failed!\n", INTERFACE_LAYER_TEST);
    //     return -1;
    // }

    // if(!test_unlink())
    // {
    //     printf("%s : Testing altfs_unlink() failed!\n", INTERFACE_LAYER_TEST);
    //     return -1;
    // }

    // if(!test_chmod())
    // {
    //     printf("%s : Testing altfs_chmod() failed!\n", INTERFACE_LAYER_TEST);
    //     return -1;
    // }

    // if(!test_permissions())
    // {
    //     printf("%s : Testing permission control failed!\n", INTERFACE_LAYER_TEST);
    //     return -1;
    // }

    printf("=============== ALL TESTS RUN =============\n\n");
    return 0;
}

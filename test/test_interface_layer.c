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
    printf("\n########## %s : Testing getattr() ##########\n", INTERFACE_LAYER_TEST);
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

    // Test for root /
    printf("TEST 1\n");
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

    // Test for /dir1
    printf("TEST 2\n");
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

    // Test for /dir1/file1
    printf("TEST 3\n");
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

    // Test for /file2
    printf("TEST 4\n");
    if(altfs_getattr("/file2", &st) != -ENOENT)
    {
        fprintf(stderr, "%s : Should have failed for /file2 but returned passed.\n", INTERFACE_LAYER_TEST);
        altfs_free_memory(st);
        return false;
    }
    altfs_free_memory(st);

    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_access()
{
    printf("\n########## %s : Testing access() ##########\n", INTERFACE_LAYER_TEST);
    
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

    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_mkdir()
{
    printf("\n########## %s : Testing mkdir() ##########\n", INTERFACE_LAYER_TEST);
    
    // Already exists
    printf("TEST 1\n");
    if(altfs_mkdir("/dir1", DEFAULT_PERMISSIONS))
    {
        fprintf(stderr, "%s : Directory already existed, but did not fail.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Parent does not exist
    printf("TEST 2\n");
    if(altfs_mkdir("/dir2/dir1", DEFAULT_PERMISSIONS))
    {
        fprintf(stderr, "%s : Parent does not exist, but did not fail.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Should succeed
    printf("TEST 3\n");
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
    printf("TEST 4\n");
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
    printf("TEST 5\n");
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
    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_mknod()
{
    printf("\n########## %s : Testing mknod() ##########\n", INTERFACE_LAYER_TEST);
    
    printf("TEST 1\n");
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
    printf("\n");

    printf("TEST 2\n");
    if(altfs_mknod("/dir3/file1", S_IFREG|S_IRUSR|S_IRGRP|S_IROTH, -1))
    {
        fprintf(stderr, "%s : Failed to flag parent does not exist for file /dir3/file1.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    
    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_open()
{
    printf("\n########## %s : Testing open() ##########\n", INTERFACE_LAYER_TEST);

    // Open non existing file
    printf("TEST 1\n");
    if(altfs_open("/file3", O_RDONLY) != -ENOENT)
    {
        fprintf(stderr, "%s : Did not fail for non existing file /file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Open non existing file with O_CREAT
    printf("TEST 2\n");
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
    printf("TEST 3\n");
    if(altfs_open("/dir2/file3", O_RDWR) < ROOT_INODE_NUM)
    {
        fprintf(stderr, "%s : Failed to open file /dir1/file1.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Fail becuase of permissions
    printf("TEST 4\n");
    if(altfs_open("/dir2/file2", O_WRONLY) != -EACCES)
    {
        fprintf(stderr, "%s : Permissions did not check correctly for /dir2/file2.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    
    // TODO: Add test for truncate
    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_write()
{
    printf("\n########## %s : Testing write() ##########\n", INTERFACE_LAYER_TEST);

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
    printf("TEST 5\n");
    if(altfs_write("/dir2/file3", data, 12, 8190) != 12)
    {
        fprintf(stderr, "%s : Did not write full string to /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    file = get_inode(inum);
    if(file->i_blocks_num != 3 || file->i_file_size != 8202)
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

    // Will write accross 3 datablocks (all existing)
    printf("TEST 6\n");
    char big_data[4106]; // 5 + 4096 + 5
    memset(big_data, 'a', 4106);
    if(altfs_write("/dir2/file3", big_data, 4106, 4091) != 4106)
    {
        fprintf(stderr, "%s : Did not write full string to /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    file = get_inode(inum);
    if(file->i_blocks_num != 3 || file->i_file_size != 8202)
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

    // Will write accross 3 datablocks (all new, starting in between the first new block)
    printf("TEST 7\n");
    if(altfs_write("/dir2/file3", big_data, 4106, 16379) != 4106)
    {
        fprintf(stderr, "%s : Did not write full string to /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    file = get_inode(inum);
    if(file->i_blocks_num != 6 || file->i_file_size != 20485)
    {
        fprintf(stderr, "%s : File size for /dir2/file3 not correct. n_blocks: %ld, size (bytes): %ld\n", INTERFACE_LAYER_TEST, file->i_blocks_num, file->i_file_size);
        altfs_free_memory(file);
        return false;
    }
    buff = read_data_block(file->i_direct_blocks[3]);
    if(strncmp("aaaaa", buff + 4091, 5) != 0)
    {
        fprintf(stderr, "%s : Incorrect data written in block 0 in /dir2/file3. Should be: |%.5s|, was: |%.5s|\n", INTERFACE_LAYER_TEST, "aaaaa", buff);
        altfs_free_memory(file);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(buff);
    buff = read_data_block(file->i_direct_blocks[4]);
    if(strncmp(big_data, buff, 4096) != 0)
    {
        fprintf(stderr, "%s : Incorrect data written in block 1 in /dir2/file3. Should be: all a's, was: |%s|\n", INTERFACE_LAYER_TEST, buff);
        altfs_free_memory(file);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(buff);
    buff = read_data_block(file->i_direct_blocks[5]);
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
    
    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_read()
{
    printf("\n########## %s : Testing read() ##########\n", INTERFACE_LAYER_TEST);
    char* buffer = (char*)calloc(1, 24);

    // Tests assume the file was opened for reading before so the file exists.
    printf("TEST 1\n");
    if(altfs_read("/dir2/file3", buffer, 0, 0) != 0)
    {
        fprintf(stderr, "%s : Did not return 0 when tried to read 0 bytes.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(altfs_read("/dir2/file3", buffer, 1, -1) != -EINVAL)
    {
        fprintf(stderr, "%s : Did not return error when tried to read at negative offset.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(altfs_read("/dir2/file3", buffer, 1, 20486) != -EOVERFLOW)
    {
        fprintf(stderr, "%s : Did not return error when tried to read at overflow offset.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Read from a single block
    printf("TEST 2\n");
    if(altfs_read("/dir2/file3", buffer, 12, 0) != 12)
    {
        fprintf(stderr, "%s : Did not read first 12 bytes fully from /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(strncmp(buffer, "Hello world!", 12) != 0)
    {
        fprintf(stderr, "%s : Did not read data correctly from /dir2/file3. Should be: |Hello world!|, got: %.12s\n", INTERFACE_LAYER_TEST, buffer);
        return false;
    }
    if(altfs_read("/dir2/file3", buffer, 12, 11) != 12)
    {
        fprintf(stderr, "%s : Did not read 12 bytes fully from /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(strncmp(buffer, "!Hello world", 12) != 0)
    {
        fprintf(stderr, "%s : Did not read data correctly from /dir2/file3. Should be: |!Hello world|, got: %.12s\n", INTERFACE_LAYER_TEST, buffer);
        return false;
    }
    printf("\n");
    memset(buffer, 0, 24);

    // Read from 2 blocks
    printf("TEST 3\n");
    if(altfs_read("/dir2/file3", buffer, 10, 4091) != 10)
    {
        fprintf(stderr, "%s : Did not read first 10 bytes fully from /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(strncmp(buffer, "aaaaaaaaaa", 10) != 0)
    {
        fprintf(stderr, "%s : Did not read data correctly from /dir2/file3. Should be: |aaaaaaaaaa|, got: %.10s\n", INTERFACE_LAYER_TEST, buffer);
        return false;
    }
    printf("\n");
    memset(buffer, 0, 24);

    // Read from 3 blocks
    printf("TEST 4\n");
    buffer = (char*)realloc(buffer, 4106);
    if(altfs_read("/dir2/file3", buffer, 4106, 4091) != 4106)
    {
        fprintf(stderr, "%s : Did not read first 10 bytes fully from /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    char test_buf[4106];
    memset(test_buf, 'a', 4106);
    if(strncmp(buffer, test_buf, 4106) != 0)
    {
        fprintf(stderr, "%s : Did not read data correctly from /dir2/file3. Should be: all a's, got: %.4106s\n", INTERFACE_LAYER_TEST, buffer);
        return false;
    }
    printf("\n");
    memset(buffer, 0, 100);

    // Read towards end of file
    printf("TEST 5\n");
    if(altfs_read("/dir2/file3", buffer, 10, 20480) != 5)
    {
        fprintf(stderr, "%s : Did not read correct number of last bytes from /dir2/file3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(strncmp(buffer, "aaaaa", 5) != 0)
    {
        fprintf(stderr, "%s : Did not read data correctly from /dir2/file3. Should be: |aaaaa|, got: %.5s\n", INTERFACE_LAYER_TEST, buffer);
        return false;
    }

    free(buffer);
    buffer = NULL;
    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_truncate()
{
    printf("\n########## %s : Testing truncate() ##########\n", INTERFACE_LAYER_TEST);

    ssize_t inum = altfs_open("/dir2/file4", O_CREAT|O_RDWR);
    if(inum < ROOT_INODE_NUM)
    {
        fprintf(stderr, "%s : File /dir/file4 not created.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Negative tests
    printf("TEST 1\n");
    if(altfs_truncate("/dir2/file4", -1) != -EINVAL)
    {
        fprintf(stderr, "%s : Did not return error when offset is negative.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(altfs_truncate("/dir2/file4", 0) != 0)
    {
        fprintf(stderr, "%s : Did not return 0 when offset is 0 and file size is 0.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Extensibility test
    printf("TEST 2\n");
    if(altfs_truncate("/dir2/file4", 10) != 0)
    {
        fprintf(stderr, "%s : Extend on truncate failed.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    inum = name_i("/dir2/file4");
    struct inode* node = get_inode(inum);
    if(node->i_file_size != 10)
    {
        fprintf(stderr, "%s : File size is not 10. Found: %ld.\n", INTERFACE_LAYER_TEST, node->i_file_size);
        altfs_free_memory(node);
        return false;
    }
    altfs_free_memory(node);
    printf("\n");

    // Truncate to 0
    printf("TEST 3\n");
    if(altfs_truncate("/dir2/file4", 0) != 0)
    {
        fprintf(stderr, "%s : Truncate to 0 failed.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    inum = name_i("/dir2/file4");
    node = get_inode(inum);
    if(node->i_file_size != 0 || node->i_blocks_num != 0)
    {
        fprintf(stderr, "%s : File size is not 10. Found: %ld bytes, %ld blocks.\n", INTERFACE_LAYER_TEST, node->i_file_size, node->i_blocks_num);
        altfs_free_memory(node);
        return false;
    }
    altfs_free_memory(node);
    printf("\n");

    // Shortening test
    printf("TEST 4\n");
    if(altfs_truncate("/dir2/file3", 16382) != 0)
    {
        fprintf(stderr, "%s : Truncate /dir2/file3 failed.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    inum = name_i("/dir2/file3");
    struct inode* node2 = get_inode(inum);
    if(node2->i_file_size != 16382)
    {
        fprintf(stderr, "%s : File size is not 16383. Found: %ld.\n", INTERFACE_LAYER_TEST, node2->i_file_size);
        altfs_free_memory(node2);
        return false;
    }
    if(node2->i_blocks_num != 4)
    {
        fprintf(stderr, "%s : File has something other than 4 blocks. Found: %ld.\n", INTERFACE_LAYER_TEST, node2->i_blocks_num);
        altfs_free_memory(node2);
        return false;
    }
    char* buff = read_data_block(node2->i_direct_blocks[3]);
    if(strncmp("aaa\0\0", buff + 4091, 5) != 0)
    {
        fprintf(stderr, "%s : File not truncated properly. Found: |%.5s|.\n", INTERFACE_LAYER_TEST, buff + 4091);
        altfs_free_memory(node2);
        altfs_free_memory(buff);
        return false;
    }
    altfs_free_memory(node2);
    altfs_free_memory(buff);

    // Permission test
    printf("TEST 5\n");
    if(altfs_truncate("/dir2/file2", 0) != -EACCES)
    {
        fprintf(stderr, "%s : Did not fail truncate on readonly file /dir2/file2.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    
    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_unlink()
{
    printf("\n########## %s : Testing unlink() ##########\n", INTERFACE_LAYER_TEST);

    // Prep
    if(!altfs_mkdir("/dir3", DEFAULT_PERMISSIONS))
    {
        printf("Dir creation failed.\n");
        return false;
    }
    ssize_t file1_inum = altfs_open("/dir3/file1", O_CREAT | DEFAULT_PERMISSIONS);
    if(file1_inum < ROOT_INODE_NUM)
    {
        printf("File1 creation failed.\n");
        return false;
    }
    char content[50000]; // Go till single indirect block
    memset(content, 'a', 50000);
    if(altfs_write("/dir3/file1", content, 50000, 0) != 50000)
    {
        printf("Failed writing to file1\n");
        return false;
    }
    ssize_t file2_inum = altfs_open("/dir3/file2", O_CREAT | DEFAULT_PERMISSIONS);
    if(file2_inum < ROOT_INODE_NUM)
    {
        printf("File2 creation failed.\n");
        return false;
    }
    printf("\n");

    // Validation failures
    printf("TEST 1\n");
    if(altfs_unlink("/dir1/file5") != -ENOENT)
    {
        fprintf(stderr, "%s : Did not flag non-existing file.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(altfs_unlink("/dir3") != -ENOTEMPTY)
    {
        fprintf(stderr, "%s : Did not flag non-empty directory /dir2.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Remove File 1
    printf("TEST 2\n");
    unsigned long long free_blocks_init = get_num_of_free_blocks();
    if(altfs_unlink("/dir3/file1") != 0)
    {
        fprintf(stderr, "%s : Failed to unlink /dir3/file1.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    unsigned long long free_blocks_final = get_num_of_free_blocks();
    ssize_t blocks_freed = free_blocks_final - free_blocks_init;
    // Verify manually in logs. Should have freed 14 - #of blocks allocated to freelist.
    printf("%s : Should have freed 14 blocks. Freed: %ld\n", INTERFACE_LAYER_TEST, blocks_freed);

    struct inode* file1 = get_inode(file1_inum);
    if(file1->i_allocated || file1->i_file_size != 0 || file1->i_blocks_num != 0)
    {
        fprintf(stderr, "%s : Failed free inode %ld for file /dir3/file1.\n", INTERFACE_LAYER_TEST, file1_inum);
        altfs_free_memory(file1);
        return false;
    }
    altfs_free_memory(file1);
    ssize_t dir_inum = name_i("/dir3");
    struct inode* dir = get_inode(dir_inum);
    if(dir->i_child_num != 3)
    {
        fprintf(stderr, "%s : Child count for /dir3 is not 3: %ld.\n", INTERFACE_LAYER_TEST, dir->i_child_num );
        altfs_free_memory(dir);
        return false;
    }
    altfs_free_memory(dir);
    printf("\n");

    // Remove File 2
    printf("TEST 3\n");
    if(altfs_unlink("/dir3/file2") != 0)
    {
        fprintf(stderr, "%s : Failed to unlink /dir3/file2.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    struct inode* file2 = get_inode(file2_inum);
    if(file2->i_allocated || file2->i_file_size != 0 || file2->i_blocks_num != 0)
    {
        fprintf(stderr, "%s : Failed free inode %ld for file /dir3/file2.\n", INTERFACE_LAYER_TEST, file2_inum);
        altfs_free_memory(file2);
        return false;
    }
    altfs_free_memory(file2);
    dir = get_inode(dir_inum);
    if(dir->i_child_num != 2)
    {
        fprintf(stderr, "%s : Child count for /dir3 is not 2: %ld.\n", INTERFACE_LAYER_TEST, dir->i_child_num );
        altfs_free_memory(dir);
        return false;
    }
    altfs_free_memory(dir);
    printf("\n");

    // Remove Dir 3
    printf("TEST 4\n");
    if(altfs_unlink("/dir3") != 0)
    {
        fprintf(stderr, "%s : Failed to unlink /dir3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    dir = get_inode(dir_inum);
    if(dir->i_allocated || dir->i_file_size != 0 || dir->i_blocks_num != 0)
    {
        fprintf(stderr, "%s : Failed free inode %ld for directory /dir3.\n", INTERFACE_LAYER_TEST, dir_inum);
        altfs_free_memory(dir);
        return false;
    }
    altfs_free_memory(dir);
    struct inode* root = get_inode(ROOT_INODE_NUM);
    if(root->i_child_num != 4)
    {
        fprintf(stderr, "%s : Child count for / is not 4: %ld.\n", INTERFACE_LAYER_TEST, root->i_child_num );
        altfs_free_memory(root);
        return false;
    }
    altfs_free_memory(root);
    printf("\n");

    // Verify removed from inode cache
    printf("TEST 5\n");
    if(name_i("/dir3") != -1)
    {
        fprintf(stderr, "%s : Failed to empty inode cache for /dir3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(name_i("/dir3/file1") != -1)
    {
        fprintf(stderr, "%s : Failed to empty inode cache for /dir3/file1.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Try to remove root
    printf("TEST 6\n");
    if(altfs_unlink("/") != -EACCES)
    {
        fprintf(stderr, "%s : Failed to flag root deletion.\n", INTERFACE_LAYER_TEST);
        return false;
    }

    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_rename()
{
    printf("\n########## %s : Testing rename() ##########\n", INTERFACE_LAYER_TEST);

    // Prepare
    if(altfs_write("/dir2/file4", "This is a test string.", 22, 0) != 22)
    {
        fprintf(stderr, "%s : Failed to write to /dir2/file4.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // The target path does not have valid parent
    printf("TEST 1\n");
    if(altfs_rename("/dir2/file4", "/dir3/file1") != -ENOENT)
    {
        fprintf(stderr, "%s : Failed to flag non existing parent for destination.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(altfs_rename("/dir2/file4", "/dir2/dir3/file1") != -ENOENT)
    {
        fprintf(stderr, "%s : Failed to flag non existing parent for destination 2.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    printf("\n");

    // Rename should be successful
    printf("TEST 2\n");
    ssize_t initial_inum = name_i("/dir2/file4");
    if(altfs_rename("/dir2/file4", "/dir2/file1") != 0)
    {
        fprintf(stderr, "%s : Failed to rename /dir2/file4 to /dir2/file1.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    ssize_t inum1 = name_i("/dir2/file4");
    if(inum1 != -1)
    {
        fprintf(stderr, "%s : Failed to remove /dir2/file4.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    ssize_t inum2 = name_i("/dir2/file1");
    if(inum2 == -1)
    {
        fprintf(stderr, "%s : Failed to add /dir2/file1.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(inum2 != initial_inum)
    {
        fprintf(stderr, "%s : Inode number changed from %ld to %ld.\n", INTERFACE_LAYER_TEST, initial_inum, inum2);
        return false;
    }
    struct inode* node = get_inode(inum2);
    if(node->i_file_size != 22)
    {
        fprintf(stderr, "%s : The file /dir2/file1 should have 22 bytes. Has: %ld.\n", INTERFACE_LAYER_TEST, node->i_file_size);
        altfs_free_memory(node);
        return false;
    }
    altfs_free_memory(node);
    printf("\n");

    // Rename larger file
    printf("TEST 3\n");
    if(!altfs_mkdir("/dir2/dir3", DEFAULT_PERMISSIONS))
    {
        fprintf(stderr, "%s : Failed to create directory /dir2/dir3.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    if(altfs_rename("/dir2/file3", "/dir2/dir3/file1") != 0)
    {
        fprintf(stderr, "%s : Failed to rename /dir2/file4 to /dir2/file1.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    inum1 = name_i("/dir2/dir3/file1");
    if(inum1 == -1)
    {
        fprintf(stderr, "%s : Failed to add /dir2/dir3/file1.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    node = get_inode(inum1);
    if(node->i_file_size != 16382)
    {
        fprintf(stderr, "%s : The file /dir2/dir3/file1 should have 16382 bytes. Has: %ld.\n", INTERFACE_LAYER_TEST, node->i_file_size);
        altfs_free_memory(node);
        return false;
    }
    altfs_free_memory(node);
    printf("\n");

    // Rename a directory (should not affect children)
    printf("TEST 4\n");
    if(altfs_rename("/dir2/dir3", "/dir2/dir1") != 0)
    {
        fprintf(stderr, "%s : Failed to rename /dir2/dir3 to /dir2/dir1.\n", INTERFACE_LAYER_TEST);
        return false;
    }
    inum1 = name_i("/dir2/dir3/file1");
    inum2 = name_i("/dir2/dir1/file1");
    if(inum1 != -1 || inum2 == -1)
    {
        fprintf(stderr, "%s : Wrong inums fetched for /dir2/dir3/file1: %ld (should be -1), or /dir2/dir1/file1: %ld (should not be -1).\n", INTERFACE_LAYER_TEST, inum1, inum2);
        return false;
    }
    
    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_chmod()
{
    printf("\n########## %s : Testing chmod() ##########\n", INTERFACE_LAYER_TEST);

    
    
    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
    return true;
}

bool test_permissions()
{
    printf("\n########## %s : Testing permissions ##########\n", INTERFACE_LAYER_TEST);

    
    
    printf("########## %s : Done! ##########\n", INTERFACE_LAYER_TEST);
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

    if(!test_read())
    {
        printf("%s : Testing altfs_read() failed!\n", INTERFACE_LAYER_TEST);
        return -1;
    }

    if(!test_truncate())
    {
        printf("%s : Testing altfs_truncate() failed!\n", INTERFACE_LAYER_TEST);
        return -1;
    }

    if(!test_unlink())
    {
        printf("%s : Testing altfs_unlink() failed!\n", INTERFACE_LAYER_TEST);
        return -1;
    }

    if(!test_rename())
    {
        printf("%s : Testing altfs_rename() failed!\n", INTERFACE_LAYER_TEST);
        return -1;
    }

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

    printf("\n=============== ALL TESTS RUN =============\n\n");
    return 0;
}

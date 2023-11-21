#include<stdio.h>
#include "../src/disk_layer.c"

#define SUPERBLOCK_LAYER_TEST "altfs_superblock_layer_test"
#define SUCCESS "Success: "
#define FAILED "Failed: "

void print_superblock()
{
    char *buffer = (char*)malloc(BLOCK_SIZE);
    // read block 0 = superblock
    if (!altfs_read_block(0, buffer))
    {
        fprintf(stderr, "%s : Failed to read block 0 for superblock\n", SUPERBLOCK_LAYER_TEST);
        return;
    }
    struct superblock *superblockObj = (struct superblock*)buffer;
    printf("\n******************** SUPERBLOCK ********************\n");
    printf("\nNUM OF INODES: %ld \n NEXT AVAILABLE INODE: %ld \n FREE LIST HEAD: %ld \n INODE SIZE: %ld \n INODES PER BLOCK: %ld \n", superblockObj->s_inodes_count, superblockObj->s_first_ino, superblockObj->s_freelist_head, superblockObj->s_inode_size, superblockObj->s_num_of_inodes_per_block);
    printf("\n******************** SUPERBLOCK ********************\n");
    return;
}

int main(int argc, char *argv[])
{
    // Test1 : Call makefs
    bool makefs = altfs_makefs();
    if (!makefs)
    {
        fprintf(stderr, "%s Test1: %s Failed during makefs\n",SUPERBLOCK_LAYER_TEST,FAILED);
        return -1;
    }
    fprintf(stderr, "%s Test1: %s Makefs completed\n",SUPERBLOCK_LAYER_TEST,SUCCESS);

    // Test2 : Verify superblock initialization
    print_superblock();
    fprintf(stdout, "%s Test2: %s Printed superblock contents\n",SUPERBLOCK_LAYER_TEST, SUCCESS);


    return 0;
}
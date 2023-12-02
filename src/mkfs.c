#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "disk_layer.c"
#include "superblock_layer.c"

int main(int argc, char *argv[])
{
    printf("mkaltfs (01-Dec-2023)\n");
    ssize_t n_inodes = INODE_BLOCK_COUNT * (BLOCK_SIZE / sizeof(struct inode));
    printf("Creating filesystem with %ld 4k blocks and %ld inodes\n", BLOCK_COUNT, n_inodes);
    if(!altfs_makefs())
    {
        printf("Altfs makefs failed!\n");
        return 0;
    }
    if(!altfs_dealloc_memory())
    {
        printf("Failed to close fd for device.\n");
        return 0;
    }
    printf("Mkfs complete!\n");
    return 0;
}

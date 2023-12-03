#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "disk_layer.c"
#include "superblock_layer.c"

void usage()
{
    printf("Usage:\n");
    printf("mkaltfs [options]\n\n");
    printf("Make an AltFS filesystem.\n");
    printf("No options are required if the intent is to only format the device.\n\n");
    printf("Options:\n");
    printf("-h \t Display this help\n");
    printf("-E \t Erase all contents on device (without -F, this will only erase the disk)\n");
    printf("-F \t Format device to AltFS (without -E, this is same as no flag)\n");
}

int main(int argc, char *argv[])
{
    int opt;
    bool E = false; // Erase all contents on disk
    bool F = false; // Format as AltFS
    while ((opt = getopt(argc, argv, "EFh")) != -1)
    {
        switch(opt)
        {
            case 'E':
                E = true;
                break;
            case 'F':
                F = true;
                break;
            case 'h':
                usage();
                return 0;
            case '?':
                printf("Unrecognized flag!\n\nSee mkfs -h for usage.\n");
                return 1;
        }
    }
    if(!E && !F)
    {
        F = true;
    }

    printf("mkaltfs (02-Dec-2023)\n");

    ssize_t n_inodes = INODE_BLOCK_COUNT * (BLOCK_SIZE / sizeof(struct inode));
    if(F)
        printf("Creating filesystem with %ld 4k blocks and %ld inodes\n\n", BLOCK_COUNT, n_inodes);

    if(!altfs_makefs_options(E, F))
    {
        printf("Altfs makefs failed!\n");
        return 0;
    }
    if(!altfs_dealloc_memory())
    {
        printf("Failed to close fd for device.\n");
        return 0;
    }
    printf("\nMkfs complete!\n");
    return 0;
}

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
    printf("Starting mkfs...\n");
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

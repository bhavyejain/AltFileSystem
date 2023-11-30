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
#include "../src/directory_ops.c"
#include "../src/interface_layer.c"

#include "test_helpers.c"

#define INTERFACE_LAYER_TEST "test_interface_layer"


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

    printf("=============== ALL TESTS RUN =============\n\n");
    return 0;
}

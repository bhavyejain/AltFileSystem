#include "../header/superblock_layer.h"
#include "../header/disk_layer.h"

/*
Print the inode.
*/
void print_inode(struct inode** node)
{
    printf("\n******************** INODE ********************\n");
    for(ssize_t i = 0; i < NUM_OF_DIRECT_BLOCKS; i++)
    {
        printf("Direct block %ld: %ld\n", i, (*node)->i_direct_blocks[i]);
    }
    printf("Single indirect block: %ld\n", (*node)->i_single_indirect);
    printf("Double indirect block: %ld\n", (*node)->i_double_indirect);
    printf("Triple indirect block: %ld\n", (*node)->i_triple_indirect);
    printf("******************** INODE ********************\n\n");
}

/*
Print the freelist starting brom block blocknum.
*/
int print_freelist(ssize_t blocknum)
{   
    printf("\n******************** FREELIST ********************\n");
    printf("\n************ FREELIST FOR BLOCK: %ld *************\n",blocknum);
    char *buff = (char*)malloc(BLOCK_SIZE);

    if (!altfs_read_block(blocknum, buff))
    {
        printf("Print freelist: Error reading contents of free list block number: %ld\n",blocknum);
        return -1;
    }
    ssize_t *buff_numptr = (ssize_t *)buff;
    for (size_t i = 0; i < NUM_OF_ADDRESSES_PER_BLOCK / 8; i++)
    {
        for (size_t j = 0; j < 8; j++)
        {
            if (i == 0 && j == 0)
                printf("Next free block: %ld\n", buff_numptr[0]);
            else
            printf("%ld ", buff_numptr[i * 8 + j]);
        }
        printf("\n");
    }
    printf("\n*************************************************\n");
    return buff_numptr[0];
}

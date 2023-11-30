#ifndef __TEST_HELPERS__
#define __TEST_HELPERS__

#ifdef __DISK_LAYER__

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

#ifdef __SUPERBLOCK_LAYER__

void print_constants()
{
    printf("\n******************** CONSTANTS ********************\n");
    printf("\n BLOCK COUNT: %ld \n NUM_OF_DATA_BLOCKS: %ld \n NUM OF DIRECT BLOCKS: %ld \n BLOCK SIZE: %ld \n INODES PER BLOCK: %ld\n INODE BLOCK COUNT: %ld\n",BLOCK_COUNT, NUM_OF_DATA_BLOCKS, NUM_OF_DIRECT_BLOCKS, BLOCK_SIZE, (BLOCK_SIZE) / sizeof(struct inode), INODE_BLOCK_COUNT);
    printf("\n***************************************************\n\n");
}

void print_superblock(struct superblock *superblockObj)
{
    printf("\n******************** SUPERBLOCK ********************\n");
    printf("\n NUM OF INODES: %ld \n NEXT AVAILABLE INODE: %ld \n FREE LIST HEAD: %ld \n INODES PER BLOCK: %ld \n", superblockObj->s_inodes_count, superblockObj->s_first_ino, superblockObj->s_freelist_head, superblockObj->s_num_of_inodes_per_block);
    printf("\n****************************************************\n");
    return;
}

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

#ifdef __DATA_BLOCK_OPS__
#ifdef __INODE_OPS__

/*
Print records in a directory.

@param iblock: If -1, print all records in all allocated datablocks. Otherwise, print records from logical datablock iblock.
*/
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
            else if((ch >= 'a' && ch <= 'z') || (ch == '_') || (ch >= '0' && ch <= '9') || (ch == '.'))
                printf("%c", ch);
        }
        iblock++;
    }

    printf("\n==========================\n");
    printf("\n");
}

#endif // __INODE_OPS__
#endif // __DATA_BLOCK_OPS__
#endif // __SUPERBLOCK_LAYER__
#endif // __DISK_LAYER__
#endif
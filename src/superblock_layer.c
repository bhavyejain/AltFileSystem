#include "../header/disk_layer.h"
#include "../header/superblock_layer.h"

/*
Write the superblock (static) to the disk.

@return true if success, false if failure.
*/
bool altfs_write_superblock()
{
    char buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);
    memcpy(buffer, altfs_superblock, sizeof(struct superblock));
    if(!altfs_write_block(0, buffer))
    {
        fuse_log(FUSE_LOG_ERR, "%s Error writing superblock to memory.\n", ALTFS_SUPERBLOCK);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Superblock written!!!\n", ALTFS_SUPERBLOCK);
    return true;
}

/*
Reads the first block of memory and assigns it to the static superblock object.

@return true if success, false if failure.
*/
bool load_superblock()
{
    char sb_buffer[BLOCK_SIZE];
    memset(sb_buffer, 0, BLOCK_SIZE);
    altfs_read_block(0, sb_buffer);
    char empty_buffer[BLOCK_SIZE];
    memset(empty_buffer, 0, BLOCK_SIZE);
    if(memcmp(sb_buffer, empty_buffer, BLOCK_SIZE) == 0)    // need to run mkfs
    {
        fuse_log(FUSE_LOG_ERR, "load_superblock : Superblock empty!!!\n");
        return false;
    }

    struct superblock* sb = (struct superblock*)sb_buffer;
    ssize_t t1 = (BLOCK_SIZE) / sizeof(struct inode);
    ssize_t t2 = INODE_BLOCK_COUNT * t1;
    if(sb->s_num_of_inodes_per_block != t1 || sb->s_inodes_count != t2)
    {
        fuse_log(FUSE_LOG_ERR, "load_superblock : Superblock incorrect!!!\n");
        return false;
    }

    if(altfs_superblock == NULL)
    {
        altfs_superblock = (struct superblock*)malloc(sizeof(struct superblock));
    }
    memcpy(altfs_superblock, sb, sizeof(struct superblock));
    fuse_log(FUSE_LOG_DEBUG, "load_superblock : Superblock loaded!\n");
    return true;
}

// Initializes superblock and writes to physical block 0
bool altfs_create_superblock()
{
    altfs_superblock = (struct superblock*)malloc(sizeof(struct superblock));
    // fd 0,1,2 = input, output, error => first ino will start from 3
    altfs_superblock->s_first_ino = 3;
    //altfs_superblock->s_inode_size = sizeof(struct inode);
    altfs_superblock->s_num_of_inodes_per_block = (BLOCK_SIZE) / sizeof(struct inode);
    altfs_superblock->s_inodes_count = INODE_BLOCK_COUNT * (altfs_superblock->s_num_of_inodes_per_block);
    // first data block will be the head of free list
    altfs_superblock->s_freelist_head = INODE_BLOCK_COUNT + 1;

    fuse_log(FUSE_LOG_DEBUG, "%s Writing superblock for the first time...\n", ALTFS_SUPERBLOCK);
    return altfs_write_superblock();
}

// Creates ilist by initializing inode struct for all inodes
// Does a write operation block wise for the number of inodes contained in a block
bool altfs_create_ilist()
{
    // TODO: Verify all elements of struct are getting initialized correctly
    // Below line creates an array with just 1 element whose size = size of inode struct
    // The initialization initializes the first element of the array (the array just has 1 element)
    struct inode node;
    node.i_mode = 0;
    node.i_uid = 0;
    node.i_gid = 0;
    node.i_status_change_time = 0;
    node.i_atime = 0;
    node.i_ctime = 0;
    node.i_mtime = 0;
    node.i_links_count = 0;
    node.i_file_size = 0;
    node.i_blocks_num = 0;
    node.i_allocated = false;
    for(ssize_t i = 0; i < NUM_OF_DIRECT_BLOCKS; i++)
        node.i_direct_blocks[i] = 0;
    node.i_single_indirect = 0;
    node.i_double_indirect = 0;
    node.i_triple_indirect = 0;
    node.i_child_num = 0;
    
    char buffer[BLOCK_SIZE];
    fuse_log(FUSE_LOG_DEBUG, "%s Creating ilist...\n", ALTFS_CREATE_ILIST);
    // initialize ilist for all blocks meant for inodes
    // start with index = 1 since superblock will take block 0
    for(ssize_t blocknum = 1; blocknum <= INODE_BLOCK_COUNT; blocknum++)
    {
        ssize_t offset = 0;
        memset(buffer, 0, BLOCK_SIZE);
        for(ssize_t inodenum = 0; inodenum < altfs_superblock->s_num_of_inodes_per_block; inodenum++)
        {
            memcpy(buffer + offset, &node, sizeof(struct inode));
            offset += sizeof(struct inode);
        }
        //fuse_log(FUSE_LOG_DEBUG, "%s Writing blocknum %ld and buffer %s\n",ALTFS_CREATE_ILIST, blocknum, *buffer);
        if (!altfs_write_block(blocknum, buffer)){
            fuse_log(FUSE_LOG_ERR, "%s Error writing to inode block number %d\n", ALTFS_CREATE_ILIST, blocknum);
            return false;
        }
    }
    return true;
}


bool altfs_create_freelist()
{
    // iterate over all blocks meant for free list 
    // Initialize the addresses
    ssize_t blocknum = altfs_superblock->s_freelist_head;
    char buffer[BLOCK_SIZE];
    ssize_t nullvalue = 0;

    for(ssize_t i = 0; i < NUM_OF_FREE_LIST_BLOCKS; i++)
    {
        ssize_t currblocknum = blocknum;
        // initialize block with zeroes
        memset(buffer, 0, BLOCK_SIZE);
        ssize_t offset = 0;
        // for each block, initialize addresses
        // Starts from index 1 since first one witll have block address off next free list block
        for(ssize_t j=1; j < NUM_OF_ADDRESSES_PER_BLOCK; j++)
        {
            offset += ADDRESS_SIZE; 
            blocknum += 1;
            // start writing at buffer + offset since first index will have address of next free block
            // Copy address of the respective block into the buffer
            if (blocknum >= BLOCK_COUNT)
                memcpy(buffer+offset, &nullvalue, ADDRESS_SIZE);
            else
                memcpy(buffer+offset, &blocknum, ADDRESS_SIZE);
        }
        blocknum += 1;
        if (blocknum >= BLOCK_COUNT)
            memcpy(buffer, &nullvalue, ADDRESS_SIZE);
        else
            memcpy(buffer, &blocknum, ADDRESS_SIZE);
        
        if (!altfs_write_block(currblocknum, buffer))  
        {
            fuse_log(FUSE_LOG_ERR, "%s Error writing block number %d to free list\n", ALTFS_CREATE_FREELIST, currblocknum);
            return false;
        }
        //fuse_log(FUSE_LOG_DEBUG, "%s Successfully wrote block number %ld with contents %s\n", ALTFS_CREATE_FREELIST, currblocknum, buffer);
        // TODO: Check on below check
        //if (blocknum >= BLOCK_COUNT)
        //    break;
    }
    return true;
}

bool altfs_makefs()
{
    // TODO: Check if this is remounting 
    // if yes, read superblock and skip init steps
    bool allocateFSMemory = altfs_alloc_memory();
    if (!allocateFSMemory)
    {
        fuse_log(FUSE_LOG_ERR, "%s Error allocating memory while initializing FS\n",ALTFS_MAKEFS);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Allocated memory for FS while initializing\n", ALTFS_MAKEFS);

    bool createSuperBlock = altfs_create_superblock();
    if (!createSuperBlock)
    {
        fuse_log(FUSE_LOG_ERR, "%s Error creating superblock while initializing FS\n",ALTFS_MAKEFS);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Successfully created superblock for FS\n", ALTFS_MAKEFS);

    bool createInodeList = altfs_create_ilist();
    if (!createInodeList)
    {
        fuse_log(FUSE_LOG_ERR, "%s Error creating inode list while initializing FS\n",ALTFS_MAKEFS);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Successfully created inode list for FS\n", ALTFS_MAKEFS);

    bool createFreeList = altfs_create_freelist();
    if (!createFreeList)
    {
        fuse_log(FUSE_LOG_ERR, "%s Error creating free list while initializing FS\n",ALTFS_MAKEFS);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s Successfully created free list for FS\n", ALTFS_MAKEFS);
    return true;
}

void teardown()
{
    if(!altfs_write_superblock())
    {
        fuse_log(FUSE_LOG_ERR, "teardown : Failed to write superblock!\n");
    }
    fuse_log(FUSE_LOG_ERR, "teardown : Freeing superblock!\n");
    altfs_free_memory(altfs_superblock);
    fuse_log(FUSE_LOG_ERR, "teardown : Freeing mem disk!\n");
    if(!altfs_dealloc_memory())
    {
        fuse_log(FUSE_LOG_ERR, "teardown : Failed to deallocate memory!\n");
    }
}

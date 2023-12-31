#include <stdarg.h>
#include <unistd.h>

#include "../header/disk_layer.h"

#ifdef DISK_MEMORY
    static ssize_t mem_ptr;
#else 
    static char *mem_ptr;
#endif 

bool altfs_alloc_memory(bool erase)
{
    #ifdef DISK_MEMORY
        mem_ptr = open(DEVICE_NAME, O_RDWR);
        if (mem_ptr == -1)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Error opening device %s\n", ALTFS_ALLOC_MEMORY, DEVICE_NAME);
            return false;
        }
        fuse_log(FUSE_LOG_DEBUG, "%s : Ready to format disk\n", ALTFS_ALLOC_MEMORY);
        
        // Initialize all blocks with zero
        if(erase)
        {
            fuse_log(FUSE_LOG_DEBUG, "%s : Erasing device contents...\n", ALTFS_ALLOC_MEMORY);
            char buff[BLOCK_SIZE];
            memset(&buff, 0, BLOCK_SIZE);
            off_t lseek_status = lseek(mem_ptr, 0, SEEK_SET);
            if(lseek_status == -1)
            {
                fuse_log(FUSE_LOG_ERR, "%s : lseek to 0 failed.\n", ALTFS_ALLOC_MEMORY);
                return false;
            }
            for(ssize_t i=0; i < BLOCK_COUNT; i++)
            {
                if(write(mem_ptr, buff, BLOCK_SIZE) != BLOCK_SIZE)
                {
                    fuse_log(FUSE_LOG_ERR, "%s: Formatting the disk failed!!\n", ALTFS_WRITE_BLOCK);
                    return false;
                }
                printf("Erased: %d %%\r", (int)(((i + 1.0)/BLOCK_COUNT)*100));
                fflush(stdout);
            }
            printf("\n");
            fuse_log(FUSE_LOG_DEBUG, "%s : Erasing device contents : Done!\n", ALTFS_ALLOC_MEMORY);
        }
    #else 
        // TODO: Check that it works for all data types in files
        // TODO: check if calloc works same as malloc+memset
        // TODO: Check why this should be cast to char* and not void*
        mem_ptr = (char*)calloc(1, FS_SIZE);
        if (!mem_ptr)
        {
            fuse_log(FUSE_LOG_ERR, "%s Error allocating memory\n",ALTFS_ALLOC_MEMORY);
            return false;
        }
        fuse_log(FUSE_LOG_DEBUG, "%s Allocated memory for FS at %p\n", ALTFS_ALLOC_MEMORY, &mem_ptr);
    #endif
    return true;
}

bool altfs_open_volume()
{
    #ifdef DISK_MEMORY
    mem_ptr = open(DEVICE_NAME, O_RDWR);
    if (mem_ptr == -1)
    {
        fuse_log(FUSE_LOG_ERR, "%s : Error opening device %s\n", ALTFS_ALLOC_MEMORY, DEVICE_NAME);
        return false;
    }
    fuse_log(FUSE_LOG_DEBUG, "%s : Opened device.\n", ALTFS_ALLOC_MEMORY);
    #endif

    return true;
}

bool altfs_dealloc_memory()
{
    #ifdef DISK_MEMORY
        if (close(mem_ptr) != 0)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Error deallocating memory on disk\n", ALTFS_DEALLOC_MEMORY);
            return false;
        }
    #else
        if(mem_ptr != NULL)
            free(mem_ptr);
        else
        {
            fuse_log(FUSE_LOG_ERR, "%s : No pointer to deallocate memory\n",ALTFS_DEALLOC_MEMORY);
            return false;
        }
        fuse_log(FUSE_LOG_DEBUG, "%s : Deallocated memory\n",ALTFS_DEALLOC_MEMORY);
    #endif
    return true;
}

bool isBlockOutOfRange(ssize_t blockid)
{
    return (blockid < 0 || blockid >= BLOCK_COUNT);
}

bool altfs_read_block(ssize_t blockid, char *buffer)
{
    if (!buffer)
        return false;
    if (isBlockOutOfRange(blockid))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Error reading block from disk. Block id out of range: %ld\n", ALTFS_READ_BLOCK, blockid);
        return false;
    }
    #ifdef DISK_MEMORY
        off_t offset = (unsigned long) BLOCK_SIZE * blockid;
        off_t lseek_status = lseek(mem_ptr, offset, SEEK_SET);
        if(lseek_status == -1)
        {
            fuse_log(FUSE_LOG_ERR, "%s : lseek to offset %jd failed for block id %zd\n", ALTFS_READ_BLOCK, offset, blockid);
            return false;
        }
        if(read(mem_ptr, buffer, BLOCK_SIZE) != BLOCK_SIZE)
        {
            fuse_log(FUSE_LOG_ERR, "%s: Reading contents of disk failed\n", ALTFS_READ_BLOCK);
            return false;
        }
    #else
        ssize_t offset = blockid * BLOCK_SIZE;
        memcpy(buffer, mem_ptr+offset, BLOCK_SIZE);
    #endif // DISK_MEMORY
    return true;
}

bool altfs_write_block(ssize_t blockid, char *buffer)
{
    if (!buffer)
        return false;
    if (isBlockOutOfRange(blockid))
    {
        fuse_log(FUSE_LOG_ERR, "%s : Error writing block to disk. Block id is out of range\n",ALTFS_WRITE_BLOCK);
        return false;
    }
    
    #ifdef DISK_MEMORY
        off_t offset = (unsigned long) BLOCK_SIZE * blockid;
        off_t lseek_status = lseek(mem_ptr, offset, SEEK_SET);
        if(lseek_status == -1)
        {
            fuse_log(FUSE_LOG_ERR, "%s : lseek to offset %jd failed for block id %zd\n", ALTFS_WRITE_BLOCK, offset, blockid);
            return false;
        }
        if(write(mem_ptr, buffer, BLOCK_SIZE) != BLOCK_SIZE)
        {
            fuse_log(FUSE_LOG_ERR, "%s : Writing contents to disk failed\n", ALTFS_WRITE_BLOCK);
            return false;
        }
    #else
        ssize_t offset = BLOCK_SIZE * blockid;
        memcpy(mem_ptr+offset, buffer, BLOCK_SIZE);
    #endif

    return true;
}

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "../header/inode_cache.h"

#define TEST_INODE_CACHE "test_inode_cache"

int main()
{
    ssize_t cache_size = 5;
    static struct inode_cache inodeCache;
    create_inode_cache(&inodeCache, cache_size);

    assert(inodeCache.capacity == cache_size);
    assert(inodeCache.size == 0);

    set_cache_entry(&inodeCache, "/dir1/dir2/file1.txt", 123);
    set_cache_entry(&inodeCache, "/dir2/dir3/file2.mov", 345);
    set_cache_entry(&inodeCache, "/dir1/dir2/file3.txt", 1234);
    set_cache_entry(&inodeCache, "/dir1/dir2/dir3/file4.mov", 3456);
    set_cache_entry(&inodeCache, "/dir1/dir2/file5.txt", 1231);

    fprintf(stdout, "%s : Successfully added entries to cache\n", TEST_INODE_CACHE);

    assert(get_cache_entry(&inodeCache, "/dir1/dir2/file1.txt") == 123); 
    assert(get_cache_entry(&inodeCache, "/dir2/dir3/file2.mov") == 345); 
    assert(get_cache_entry(&inodeCache, "/dir1/dir2/file3.txt") == 1234);
    assert(get_cache_entry(&inodeCache, "/dir1/dir2/dir3/file4.mov") == 3456); 
    assert(get_cache_entry(&inodeCache, "/dir1/dir2/file5.txt") == 1231);
    assert(get_cache_entry(&inodeCache, "/dir1/file5.txt") == -1);
    
    fprintf(stdout, "%s : Successfully verified cache values\n", TEST_INODE_CACHE);

    set_cache_entry(&inodeCache, "/dir1/dir2/dir3/dir4/file6.txt", 12345); // /dir1/dir2/file1.txt will be evicted

    assert(get_cache_entry(&inodeCache, "/dir1/dir2/dir3/dir4/file6.txt") == 12345);
    assert(get_cache_entry(&inodeCache, "/dir1/dir2/file1.txt") == -1); 
    assert(get_cache_entry(&inodeCache, "/dir2/dir3/file2.mov") == 345); 
    assert(get_cache_entry(&inodeCache, "/dir1/dir2/file3.txt") == 1234);
    assert(get_cache_entry(&inodeCache, "/dir1/dir2/dir3/file4.mov") == 3456); 
    assert(get_cache_entry(&inodeCache, "/dir1/dir2/file5.txt") == 1231);
    assert(get_cache_entry(&inodeCache, "/dir1/file5.txt") == -1);

    set_cache_entry(&inodeCache, "/dir2/dir3/file7.mov", 34567); // /dir1/dir2/dir3/dir4/file6.txt will be evicted

    assert(get_cache_entry(&inodeCache, "/dir1/dir2/dir3/dir4/file6.txt") == -1);
    assert(get_cache_entry(&inodeCache, "/dir2/dir3/file7.mov") == 34567); 

    fprintf(stdout, "%s : Successfully verified cache eviction\n", TEST_INODE_CACHE);

    remove_cache_entry(&inodeCache, "/dir1/dir2/dir3/file4.mov");
    assert(get_cache_entry(&inodeCache, "/dir1/dir2/dir3/file4.mov") == -1); 

    fprintf(stdout, "%s : Successfully removed cache entry\n", TEST_INODE_CACHE);

    set_cache_entry(&inodeCache, "", 1237);

    assert(get_cache_entry(&inodeCache, "") == -1); 

    fprintf(stdout, "%s : Successfully tested adding null entries to cache\n", TEST_INODE_CACHE);

    return 0;
}
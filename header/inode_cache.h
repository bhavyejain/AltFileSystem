#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

struct cache_entry {
    struct cache_entry* prev;
    struct cache_entry* next;
    char* key;
    ssize_t value;
};

struct inode_cache {
    ssize_t size;
    ssize_t capacity;
    struct cache_entry** map;
    struct cache_entry* head;
    struct cache_entry* tail;
};

void create_inode_cache(struct inode_cache* cache, ssize_t capacity);

bool remove_cache_entry(struct inode_cache* cache, const char* key);

void set_cache_entry(struct inode_cache* cache, const char* key, ssize_t value);

ssize_t get_cache_entry(struct inode_cache* cache, const char* key);

void free_inode_cache(struct inode_cache* cache);
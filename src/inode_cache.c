#include "../../header/inode_cache.h"

// source - https://stackoverflow.com/questions/64699597/how-to-write-djb2-hashing-function-in-c
unsigned long hash_func(const char *str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

void create_inode_cache(struct inode_cache* cache, ssize_t capacity) {
    if(cache == NULL)
    {
        cache = (struct inode_cache*) malloc(sizeof(struct inode_cache));
    }
    cache->head = NULL;
    cache->tail = NULL;
    cache->size = 0;
    cache->capacity = capacity;
    cache->map = (struct cache_entry**) calloc(capacity, sizeof(struct cache_entry**));
}

void free_cache_entry(struct cache_entry* node) {
    if(node != NULL)
    {
        free(node->key);
        free(node);
    }
}

void free_list(struct cache_entry* node) {
    while (node) {
        struct cache_entry* next = node->next;
        free_cache_entry(node);
        node = next;
    }
}

void free_inode_cache(struct inode_cache* cache) {
    if(cache != NULL)
    {
        free_list(cache->head);
        free(cache->map);
        free(cache->tail);
        free(cache);
    }
}

void delete_node(struct inode_cache* cache, struct cache_entry* node) {
    // if node == tail or head of cache, update tail/head pointers
    if (node == cache->head) 
    {
        cache->head = node->next;
    } 
    else if (node == cache->tail) 
    {
        cache->tail = node->prev;
    }
    if (node->prev != NULL) {
        node->prev->next = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    free_cache_entry(node);
}

bool remove_cache_entry(struct inode_cache* cache, const char* key)
{
    if (cache == NULL || key == NULL) {
        return -1;
    }
    // Find the node with the given key
    unsigned long hash = hash_func(key);
    ssize_t hash_index = hash % cache->capacity;
    struct cache_entry* curr = cache->map[hash_index];
    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            delete_node(cache, curr);
            cache->map[hash_index] = NULL;
            return true;
        }
        curr = curr->next;
    }
    // If key is not present in cache, return false
    return false; 
}

void set_cache_entry(struct inode_cache* cache, const char* key, ssize_t value) 
{
    if (cache == NULL || key == NULL || strlen(key) == 0) {
        return;
    }
   
    unsigned long hash = hash_func(key);
    ssize_t hash_index = hash % cache->capacity;
    struct cache_entry* curr = cache->map[hash_index];

    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            curr->value = value;
            // Move the new node to front of the cache
            if (curr != cache->head) {
                curr->prev->next = curr->next;
                if (curr == cache->tail) {
                    cache->tail = curr->prev;
                } else {
                    curr->next->prev = curr->prev;
                }
                curr->prev = NULL;
                curr->next = cache->head;
                cache->head->prev = curr;
                cache->head = curr;
            }
            return;
        }
        curr = curr->next;
    }

    struct cache_entry* node = (struct cache_entry*) malloc(sizeof(struct cache_entry));
    node->key = (char*) malloc(strlen(key) + 1);
    strcpy(node->key, key);
    node->value = value;
    node->prev = NULL;
    node->next = cache->head;

    if (cache->head != NULL) {
        cache->head->prev = node;
    }
    cache->head = node;
    if (cache->tail == NULL) {
        cache->tail = node;
    }
    cache->size++;
    // If cache is full, remove the LRU cache entry
    if (cache->size > cache->capacity) {
        delete_node(cache, cache->tail);
        cache->size--;
    }
    cache->map[hash_index] = node;
}

ssize_t get_cache_entry(struct inode_cache* cache, const char* key){
    if (cache == NULL || key == NULL || strlen(key) == 0) {
        return -1;
    }

    unsigned long hash = hash_func(key);
    ssize_t hash_index = hash % cache->capacity;
    struct cache_entry* curr = cache->map[hash_index];

    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            if (curr != cache->head) {
                curr->prev->next = curr->next;
                if (curr == cache->tail) {
                    cache->tail = curr->prev;
                } else {
                    curr->next->prev = curr->prev;
                }
                curr->prev = NULL;
                curr->next = cache->head;
                cache->head->prev = curr;
                cache->head = curr;
            }
            return curr->value;
        }
        curr = curr->next;
    }
    return -1;
}

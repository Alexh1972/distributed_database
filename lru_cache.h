/*
 * Copyright (c) 2024, <>
 */

#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <stdbool.h>
#include "hash_table.h"
#include "queue.h"

typedef struct lru_cache {
    hashtable_t *ht;
	doubly_linked_list_t *list;
    unsigned int cache_capacity;
} lru_cache;

typedef struct lru_cache_information {
	void *data;
	unsigned int length;
} lru_cache_information;

lru_cache *init_lru_cache(unsigned int cache_capacity);

bool lru_cache_is_full(lru_cache *cache);

void free_lru_cache(lru_cache **cache);

/**
 * lru_cache_put() - Adds a new pair in our cache.
 * 
 * @param cache: Cache where the key-value pair will be stored.
 * @param key: Key of the pair.
 * @param value: Value of the pair.
 * @param evicted_key: The function will RETURN via this parameter the
 *      key removed from cache if the cache was full.
 * 
 * @return - true if the key was added to the cache,
 *      false if the key already existed.
 */
bool lru_cache_put(lru_cache *cache, void *key, void *value,
                   void **evicted_key);

/**
 * lru_cache_get() - Retrieves the value associated with a key.
 * 
 * @param cache: Cache where the key-value pair is stored.
 * @param key: Key of the pair.
 * 
 * @return - The value associated with the key,
 *      or NULL if the key is not found.
 */
void *lru_cache_get(lru_cache *cache, void *key, void **evicted_key);

/**
 * lru_cache_remove() - Removes a key-value pair from the cache.
 * 
 * @param cache: Cache where the key-value pair is stored.
 * @param key: Key of the pair.
*/
void lru_cache_remove(lru_cache *cache, void *key);

void print_lru_cache(lru_cache *cache);

lru_cache *init_lru_cache(unsigned int cache_capacity);

void lru_cache_free_key_and_val_function(void *data);

/**
 * create_lru_cache_information() - Creates a structure containing
 * information about the data used for future queries.
 * 
 * @param data: Query data.
 * @param length: Data length.
 */
lru_cache_information create_lru_cache_information(void *data,
												   unsigned int length);

#endif /* LRU_CACHE_H */

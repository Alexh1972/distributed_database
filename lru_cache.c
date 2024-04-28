/*
 * Copyright (c) 2024, <>
 */

#include <stdio.h>
#include <string.h>
#include "lru_cache.h"
#include "utils.h"

lru_cache *init_lru_cache(unsigned int cache_capacity)
{
	lru_cache *cache = malloc(sizeof(lru_cache));
	cache->cache_capacity = cache_capacity;
	cache->list = dll_create(sizeof(ht_info));
	cache->ht = ht_create(cache_capacity,
						  hash_string,
						  compare_strings,
						  lru_cache_free_key_and_val_function);
	return cache;
}

void lru_cache_free_key_and_val_function(void *data)
{
	ll_node_t *node = (ll_node_t *)data;
	ht_info *info = (ht_info *)node->data;
	dll_node_t *val_node = *((dll_node_t **)info->value);
	ht_info *inform = (ht_info *)val_node->data;
	free(inform->key);
	free(inform->value);
	free(inform);
	free(val_node);
	free(info->value);
	free(info->key);
	free(info);
}

bool lru_cache_is_full(lru_cache *cache)
{
	if (dll_get_size(cache->list) < cache->cache_capacity)
		return false;
	else
		return true;
}

void free_lru_cache(lru_cache **cache)
{
	ht_free((*cache)->ht);
	(*cache)->list->size = 0;
	dll_free(&(*cache)->list);
	free(*cache);
	*cache = NULL;
}

bool lru_cache_put(lru_cache *cache, void *key, void *value,
				   void **evicted_key)
{
	lru_cache_information *key_info = (lru_cache_information *)key;
	lru_cache_information *value_info = (lru_cache_information *)value;
	void *ht_val = ht_get(cache->ht, key_info->data);
	int key_exists = 0;
	if (lru_cache_is_full(cache) && evicted_key)
	{
		dll_node_t *lru_node = dll_get_nth_node(cache->list, 0);
		ht_info *lru_node_info = (ht_info *)lru_node->data;
		void *lru_key = lru_node_info->key;
		if (!ht_val)
		{
			*evicted_key = malloc(lru_node_info->key_size);
			memcpy(*evicted_key, lru_key, lru_node_info->key_size);
			dll_remove_nth_node(cache->list, 0);
			ht_remove_entry(cache->ht, lru_key);
		}
	}

	if (ht_val)
	{
		dll_node_t *val_node = *((dll_node_t **)ht_val);
		if (dll_is_head(cache->list, val_node))
			cache->list->head = cache->list->head->next;
		if (val_node->prev != NULL)
			val_node->prev->next = val_node->next;

		if (val_node->next != NULL)
			val_node->next->prev = val_node->prev;

		cache->list->size--;
		if (dll_get_size(cache->list) == 0)
			cache->list->head = NULL;
		key_exists = 1;
	}

	ht_info info;
	info.key = malloc(key_info->length);
	memcpy(info.key, key_info->data, key_info->length);
	info.value = malloc(value_info->length);
	memcpy(info.value, value_info->data, value_info->length);

	info.key_size = key_info->length;
	info.val_size = value_info->length;
	dll_add_tail(cache->list, &info);
	dll_node_t *new_cache_queue_node = dll_get_tail(cache->list);

	ht_put(cache->ht,
		   key_info->data,
		   key_info->length,
		   &new_cache_queue_node,
		   sizeof(dll_node_t *));

	if (key_exists)
		return true;
	else
		return false;
}

void *lru_cache_get(lru_cache *cache, void *key, void **evicted_key)
{
	lru_cache_information *key_info = (lru_cache_information *)key;
	void *data = ht_get(cache->ht, key_info->data);
	if (data)
	{
		dll_node_t *val_node = *((dll_node_t **)data);
		ht_info *info = (ht_info *)val_node->data;
		lru_cache_information val_info;
		val_info.data = info->value;
		val_info.length = info->val_size;
		void *ret = malloc(info->val_size);
		memcpy(ret, info->value, info->val_size);
		lru_cache_put(cache, key, &val_info, evicted_key);

		return ret;
	}

	return NULL;
}

void lru_cache_remove(lru_cache *cache, void *key)
{
	lru_cache_information *key_info = (lru_cache_information *)key;
	void *ht_val = ht_get(cache->ht, key_info->data);
	if (ht_val)
	{
		dll_node_t *val_node = *((dll_node_t **)ht_val);
		if (dll_is_head(cache->list, val_node))
			cache->list->head = cache->list->head->next;
		if (val_node->prev != NULL)
			val_node->prev->next = val_node->next;

		if (val_node->next != NULL)
			val_node->next->prev = val_node->prev;

		cache->list->size--;
		if (dll_get_size(cache->list) == 0)
			cache->list->head = NULL;
		ht_remove_entry(cache->ht, key_info->data);
	}
}

void print_lru_cache(lru_cache *cache)
{
	printf("\n--------PRINTING LRU CACHE - CAPACITY: %u--------\n",
		   cache->cache_capacity);
	print_ht(cache->ht);
	dll_node_t *cache_queue_node = cache->list->head;
	printf("--------PRINTING LRU CACHE QUEUE--------\n");
	while (cache_queue_node)
	{
		ht_info *info = cache_queue_node->data;
		printf("%s - %s\n", (char *)info->key, (char *)info->value);
		cache_queue_node = dll_get_next_node(cache->list, cache_queue_node);
	}
	printf("--------END--------\n");
}

lru_cache_information create_lru_cache_information(void *data,
												   unsigned int length)
{
	lru_cache_information lru_info;
	lru_info.data = data;
	lru_info.length = length;
	return lru_info;
}

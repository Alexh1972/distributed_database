/*
 * Copyright (c) 2024, <>
 */

#include "hash_table.h"
#include <stdlib.h>
#include <stdio.h>

hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void *),
					   int (*compare_function)(void *, void *),
					   void (*key_val_free_function)(void *))
{
	hashtable_t *ht = malloc(sizeof(hashtable_t));

	ht->buckets = malloc(sizeof(linked_list_t *) * hmax);
	ht->hmax = hmax;
	ht->hash_function = hash_function;
	ht->compare_function = compare_function;
	ht->key_val_free_function = key_val_free_function;
	ht->size = 0;
	for (unsigned int i = 0; i < hmax; i++)
	{
		ht->buckets[i] = ll_create(sizeof(ht_info));
	}
	return ht;
}

int ht_has_key(hashtable_t *ht, void *key)
{
	int ind = ht->hash_function(key) % ht->hmax;

	if (ht->buckets[ind]->head != NULL)
	{
		ll_node_t *node = ht->buckets[ind]->head;

		while (node)
		{
			ht_info *inform = (ht_info *)node->data;
			if (ht->compare_function(inform->key, key) == 0)
			{
				return 1;
			}
			node = node->next;
		}
	}

	return 0;
}

void *ht_get(hashtable_t *ht, void *key)
{
	int ind = ht->hash_function(key) % ht->hmax;

	if (ht->buckets[ind]->head != NULL)
	{
		ll_node_t *node = ht->buckets[ind]->head;

		while (node)
		{
			ht_info *inform = (ht_info *)node->data;
			if (ht->compare_function(inform->key, key) == 0)
			{
				return inform->value;
			}

			node = node->next;
		}
	}

	return NULL;
}

void *ht_remove_entry(hashtable_t *ht, void *key)
{
	int ind = ht->hash_function(key) % ht->hmax;

	if (ht->buckets[ind]->head != NULL)
	{
		ll_node_t *node = ht->buckets[ind]->head;
		int i = 0;
		while (node)
		{
			ll_node_t *next = node->next;
			ht_info *inform = (ht_info *)node->data;

			if (ht->compare_function(inform->key, key) == 0)
			{
				ll_node_t *rm_node = ll_remove_nth_node(ht->buckets[ind], i);
				ht->size--;
				if (ht->key_val_free_function)
				{
					ht->key_val_free_function(rm_node);
					free(rm_node);
					return NULL;
				}
				else
				{
					void *data = rm_node->data;
					free(rm_node);
					return data;
				}
			}
			i++;
			node = next;
		}
	}
	return NULL;
}

int ht_put(hashtable_t *ht, void *key, unsigned int key_size,
		   void *value, unsigned int value_size)
{
	int key_exists = 0;
	int ind = ht->hash_function(key) % ht->hmax;

	ht_info *inform = malloc(sizeof(ht_info));
	void *cp_key = malloc(key_size);
	memcpy(cp_key, key, key_size);

	void *cp_value = malloc(value_size);
	memcpy(cp_value, value, value_size);

	inform->key = cp_key;
	inform->value = cp_value;

	if (ht_has_key(ht, key))
	{
		ht_remove_entry(ht, key);
		key_exists = 1;
	}

	ll_add_nth_node(ht->buckets[ind], 0, inform);
	ht->size++;
	free(inform);
	return key_exists;
}

void ht_free(hashtable_t *ht)
{
	for (unsigned int i = 0; i < ht->hmax; i++)
	{
		ll_node_t *node = ht->buckets[i]->head;
		while (node)
		{
			ll_node_t *next = node->next;
			ht_info *inform = (ht_info *)node->data;
			ht_remove_entry(ht, inform->key);
			node = next;
		}
		free(ht->buckets[i]);
	}
	free(ht->buckets);
	free(ht);
}

unsigned int ht_get_size(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->size;
}

unsigned int ht_get_hmax(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->hmax;
}

void ht_free_key_val_function(void *data)
{
	ll_node_t *node = (ll_node_t *)data;
	ht_info *info = (ht_info *)node->data;
	free(info->key);
	free(info->value);
	free(info);
}

void print_ht(hashtable_t *ht)
{
	if (ht == NULL)
		return;
	printf("\n--------PRINTING HT - BUCKETS: %u--------\n", ht->hmax);
	for (unsigned int i = 0; i < ht->hmax; i++)
	{
		ll_node_t *node = ht->buckets[i]->head;
		printf("--------BUCKET - %u--------\n", i);
		while (node)
		{
			ll_node_t *next = node->next;
			ht_info *inform = (ht_info *)node->data;
			printf("--------KEY: %s--------\n", (char *)inform->key);
			node = next;
		}
	}
}

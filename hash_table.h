/*
 * Copyright (c) 2024, <>
 */

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "linked_list.h"
#include <string.h>

typedef struct ht_info ht_info;
struct ht_info {
	void *key;
	void *value;
	unsigned int key_size;
	unsigned int val_size;
};

typedef struct hashtable_t hashtable_t;
struct hashtable_t {
	linked_list_t **buckets;
	unsigned int size;
	unsigned int hmax;
	unsigned int (*hash_function)(void*);
	int (*compare_function)(void*, void*);
	void (*key_val_free_function)(void*);
};

void ht_free_key_val_function(void *data);

hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
		int (*compare_function)(void*, void*),
		void (*key_val_free_function)(void*));

int ht_has_key(hashtable_t *ht, void *key);

void *ht_get(hashtable_t *ht, void *key);

void *ht_remove_entry(hashtable_t *ht, void *key);


int ht_put(hashtable_t *ht, void *key, unsigned int key_size,
	void *value, unsigned int value_size);

void ht_free(hashtable_t *ht);

unsigned int ht_get_size(hashtable_t *ht);

unsigned int ht_get_hmax(hashtable_t *ht);

void print_ht(hashtable_t *ht);

#endif

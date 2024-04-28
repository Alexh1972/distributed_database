/*
 * Copyright (c) 2024, <>
 */

#ifndef QUEUE_H
#define QUEUE_H

#include "linked_list.h"

typedef struct queue_t queue_t;
typedef struct queue_t {
    linked_list_t *list;
} queue_t;

queue_t *init_queue(unsigned int data_size);

unsigned int get_size_queue(queue_t *queue);

int is_empty_queue(queue_t *queue);

void *peek_queue(queue_t *queue);

void push_queue(queue_t *queue, void *new_data);

ll_node_t *pop_queue(queue_t *queue);

void clear_queue(queue_t *queue);

void destroy_queue(queue_t **queue);

#endif

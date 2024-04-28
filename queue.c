/*
 * Copyright (c) 2024, <>
 */

#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

queue_t *init_queue(unsigned int data_size) {
    queue_t *new_queue = malloc(sizeof(queue_t));
    new_queue->list = ll_create(data_size);
    return new_queue;
}

unsigned int get_size_queue(queue_t *queue) {
    return ll_get_size(queue->list);
}

int is_empty_queue(queue_t *queue) {
    return get_size_queue(queue) == 0;
}

void* peek_queue(queue_t *queue) {
    return queue->list->head->data;
}

ll_node_t *pop_queue(queue_t *queue) {
    return ll_remove_nth_node(queue->list, 0);
}

void push_queue(queue_t *queue, void *new_data) {
    ll_add_nth_node(queue->list, get_size_queue(queue), new_data);
}

void clear_queue(queue_t *queue) {
    while (!is_empty_queue(queue)) {
       	ll_node_t *node = pop_queue(queue);
		free(node->data);
		free(node);
    }
}

void destroy_queue(queue_t **queue) {
    clear_queue(*queue);
    ll_free(&(*queue)->list);
    free(*queue);
}

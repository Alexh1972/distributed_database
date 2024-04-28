/*
 * Copyright (c) 2024, <>
 */

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef struct ll_node_t
{
    void *data;
    struct ll_node_t *next;
} ll_node_t;

typedef struct linked_list_t
{
    ll_node_t *head;
    unsigned int data_size;
    unsigned int size;
} linked_list_t;

typedef struct dll_node_t dll_node_t;
struct dll_node_t
{
    void *data;
    dll_node_t *prev, *next;
};

typedef struct doubly_linked_list_t doubly_linked_list_t;
struct doubly_linked_list_t
{
    dll_node_t *head;
    unsigned int data_size;
    unsigned int size;
};

unsigned int
ll_get_size(linked_list_t *list);

linked_list_t *
ll_create(unsigned int data_size);

void ll_add_nth_node(linked_list_t *list, unsigned int n, const void *new_data);

ll_node_t *
ll_remove_nth_node(linked_list_t *list, unsigned int n);

void ll_free(linked_list_t **pp_list);

unsigned int
dll_get_size(doubly_linked_list_t *list);

doubly_linked_list_t *
dll_create(unsigned int data_size);

dll_node_t *
dll_get_nth_node(doubly_linked_list_t *list, unsigned int n);

void dll_add_nth_node(doubly_linked_list_t *list,
                      unsigned int n,
                      const void *new_data);

dll_node_t *
dll_remove_nth_node(doubly_linked_list_t *list, unsigned int n);

void dll_free(doubly_linked_list_t **pp_list);

dll_node_t *dll_get_next_node(doubly_linked_list_t *list, dll_node_t *node);

void dll_add_tail(doubly_linked_list_t *list, void *new_data);

int dll_is_head(doubly_linked_list_t *list, dll_node_t *node);

dll_node_t *dll_get_tail(doubly_linked_list_t *list);

#endif

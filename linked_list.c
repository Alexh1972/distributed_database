/*
 * Copyright (c) 2024, <>
 */

#include "linked_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

unsigned int
ll_get_size(linked_list_t *list)
{
	return list->size;
}

linked_list_t *
ll_create(unsigned int data_size)
{
	linked_list_t *linked_list = malloc(sizeof(linked_list_t));
	linked_list->data_size = data_size;
	linked_list->head = NULL;
	linked_list->size = 0;
	return linked_list;
}

void ll_add_nth_node(linked_list_t *list, unsigned int n, const void *new_data)
{
	if (n > ll_get_size(list))
		n = ll_get_size(list);

	ll_node_t *new_node = malloc(sizeof(ll_node_t));

	new_node->data = malloc(list->data_size);
	memcpy(new_node->data, new_data, list->data_size);
	new_node->next = NULL;

	if (list->head == NULL)
	{
		list->head = new_node;
	}
	else
	{
		if (n == 0)
		{
			new_node->next = list->head;
			list->head = new_node;
		}
		else
		{
			ll_node_t *curr = list->head;
			while (n > 1)
			{
				curr = curr->next;
				n--;
			}
			new_node->next = curr->next;
			curr->next = new_node;
		}
	}

	list->size++;
}

ll_node_t *
ll_remove_nth_node(linked_list_t *list, unsigned int n)
{
	if (n > ll_get_size(list) - 1)
		n = ll_get_size(list) - 1;

	if (list->head == NULL)
		return NULL;

	list->size--;

	if (n == 0)
	{
		ll_node_t *node = list->head;
		list->head = list->head->next;
		return node;
	}
	else
	{
		ll_node_t *curr = list->head;
		while (n > 1)
		{
			curr = curr->next;
			n--;
		}

		ll_node_t *node = curr->next;
		curr->next = node->next;

		return node;
	}
}

void ll_free(linked_list_t **pp_list)
{
	if (*pp_list == NULL)
		return;
	int n = (*pp_list)->size;
	for (int i = n - 1; i >= 0; i--)
	{
		ll_node_t *node = ll_remove_nth_node(*pp_list, i);
		if (node)
		{
			free(node->data);
			free(node);
		}
	}

	free(*pp_list);
	*pp_list = NULL;
}

unsigned int
dll_get_size(doubly_linked_list_t *list)
{
	return list->size;
}

doubly_linked_list_t *
dll_create(unsigned int data_size)
{
	doubly_linked_list_t *dll = malloc(sizeof(doubly_linked_list_t));
	dll->head = NULL;
	dll->data_size = data_size;
	dll->size = 0;
	return dll;
}

dll_node_t *
dll_get_nth_node(doubly_linked_list_t *list, unsigned int n)
{
	if (list == NULL || list->head == NULL)
		return NULL;
	dll_node_t *node = list->head;
	while (n > 0)
	{
		node = node->next;
		n--;
	}
	return node;
}

void dll_add_nth_node(doubly_linked_list_t *list,
					  unsigned int n,
					  const void *new_data)
{
	unsigned int size = dll_get_size(list);

	dll_node_t *new = malloc(sizeof(dll_node_t));
	new->data = malloc(list->data_size);
	memcpy(new->data, new_data, list->data_size);
	new->prev = NULL;
	new->next = NULL;

	if (size <= n)
	{
		if (list->head == NULL || size == 0)
		{
			list->head = new;
			list->head->prev = list->head;
			list->head->next = list->head;
		}
		else
		{
			dll_node_t *node = dll_get_nth_node(list, dll_get_size(list) - 1);
			node->next = new;
			new->prev = node;
			list->head->prev = new;
			new->next = list->head;
		}
	}
	else
	{
		dll_node_t *node = dll_get_nth_node(list, n);

		if (n == 0)
		{
			new->prev = list->head->prev;
			new->next = list->head;
			list->head->prev->next = new;
			list->head->prev = new;
			list->head = new;
		}
		else
		{
			new->next = node;
			new->prev = node->prev;
			node->prev->next = new;
			node->prev = new;
		}
	}
	list->size++;
}

void dll_add_tail(doubly_linked_list_t *list, void *new_data)
{
	unsigned int size = dll_get_size(list);

	dll_node_t *new = malloc(sizeof(dll_node_t));
	new->data = malloc(list->data_size);
	memcpy(new->data, new_data, list->data_size);
	new->prev = NULL;
	new->next = NULL;

	if (size == 0)
	{
		list->head = new;
		list->head->prev = list->head;
		list->head->next = list->head;
	}
	else
	{
		new->prev = list->head->prev;
		new->next = list->head;
		list->head->prev->next = new;
		list->head->prev = new;
	}

	list->size++;
}

int dll_is_head(doubly_linked_list_t *list, dll_node_t *node)
{
	return list->head == node;
}

dll_node_t *dll_get_tail(doubly_linked_list_t *list)
{
	return list->head->prev;
}

dll_node_t *
dll_remove_nth_node(doubly_linked_list_t *list, unsigned int n)
{
	if (list == NULL || list->head == NULL)
		return NULL;

	if (n >= dll_get_size(list))
		n = dll_get_size(list) - 1;
	dll_node_t *node = dll_get_nth_node(list, n);

	node->next->prev = node->prev;
	node->prev->next = node->next;

	list->size--;

	if (list->size == 0)
	{
		list->head = NULL;
	}
	else if (node == list->head)
	{
		list->head = node->next;
	}

	return node;
}

void dll_free(doubly_linked_list_t **pp_list)
{
	if (*pp_list == NULL)
		return;
	if (dll_get_size(*pp_list))
	{
		dll_node_t *node = dll_get_nth_node(*pp_list, dll_get_size(*pp_list) - 1);
		int n = dll_get_size(*pp_list);
		while (n)
		{
			dll_node_t *prev = node->prev;
			free(node->data);
			n--;
			free(node);
			node = prev;
		}
	}
	free(*pp_list);
}

dll_node_t *dll_get_next_node(doubly_linked_list_t *list, dll_node_t *node)
{
	if (list == NULL || list->head == NULL)
		return NULL;

	if (node->next != list->head)
		return node->next;
	else
		return NULL;
}

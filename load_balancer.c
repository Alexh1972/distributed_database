/*
 * Copyright (c) 2024, <>
 */

#include "load_balancer.h"
#include "server.h"
#include <stdlib.h>

load_balancer *init_load_balancer(bool enable_vnodes)
{
	load_balancer *lb = malloc(sizeof(load_balancer));
	lb->enabled_vnodes = enable_vnodes;
	lb->hash_function_docs = hash_string;
	lb->hash_function_servers = hash_uint;
	lb->servers = dll_create(sizeof(server_t));
	return lb;
}

unsigned int get_number_replicas(load_balancer *main)
{
	if (main->enabled_vnodes)
		return MAX_REPLICAS;
	return 1;
}

void get_next_replica(load_balancer *main,
					  unsigned int hash,
					  server_t **server,
					  unsigned int *index)
{
	/**
	 * if the hash is not the "last" on the hash ring, then
	 * the replica searched has the lowest hash greater than
	 * the one sent through paramaters, otherwise the replica
	 * has the minimum hash compared to all replicas.
	*/
	unsigned int minimum_found = 0;
	unsigned int minimum_hash = -1;
	unsigned int minimum_server_id = -1;
	unsigned int minimum_index = 0;
	unsigned int global_minimum = -1;
	unsigned int global_minimum_index = 0;
	unsigned int global_minimum_server_id = -1;
	server_t *global_minimum_server = NULL;
	dll_node_t *current_server_node = main->servers->head;
	/**
	 * for each server and for each replica, calculate the overall minimum hash
	 * and also the lowest hash greater than the one searched.
	*/
	while (current_server_node)
	{
		server_t *current_server = get_server_load_balancer_node(current_server_node);
		for (unsigned int old_replica_idx = 0;
			 old_replica_idx < current_server->no_replicas;
			 old_replica_idx++)
		{
			if (hash < current_server->server_hash[old_replica_idx])
			{
				if (!minimum_found ||
					current_server->server_hash[old_replica_idx] < minimum_hash)
				{
					minimum_hash = current_server->server_hash[old_replica_idx];
					if (server)
						*server = current_server;
					minimum_index = old_replica_idx;
					minimum_found = 1;
					minimum_server_id = current_server->server_id;
				}
				else if (current_server->server_hash[old_replica_idx] == minimum_hash &&
						 minimum_server_id > current_server->server_id)
				{
					minimum_hash = current_server->server_hash[old_replica_idx];
					if (server)
						*server = current_server;
					minimum_index = old_replica_idx;
					minimum_found = 1;
					minimum_server_id = current_server->server_id;
				}
			}

			if (current_server->server_hash[old_replica_idx] < global_minimum)
			{
				global_minimum = current_server->server_hash[old_replica_idx];
				global_minimum_index = old_replica_idx;
				global_minimum_server = current_server;
				global_minimum_server_id = current_server->server_id;
			}
			else if (current_server->server_hash[old_replica_idx] == global_minimum &&
					 current_server->server_id < global_minimum_server_id)
			{
				global_minimum = current_server->server_hash[old_replica_idx];
				global_minimum_index = old_replica_idx;
				global_minimum_server = current_server;
				global_minimum_server_id = current_server->server_id;
			}
		}
		current_server_node = dll_get_next_node(main->servers, current_server_node);
	}

	if (!minimum_found)
	{
		minimum_index = global_minimum_index;
		if (server)
			*server = global_minimum_server;
	}

	if (index)
		*index = minimum_index;
}

void loader_add_server(load_balancer *main, int server_id, int cache_size)
{
	server_t *new_server =
	init_server(cache_size,
				server_id,
				main->hash_function_servers,
				main->hash_function_docs,
				get_number_replicas(main));
	/**
	 * For each replica of the new server, get the next server
	 * on the hash ring and transfer the documents which correspond
	 * with the new server.
	*/
	for (unsigned int new_replica_idx = 0;
		 new_replica_idx < new_server->no_replicas;
		 new_replica_idx++)
	{
		server_t *next_server = NULL;
		unsigned int minimum_index = 0;
		// getting next server
		get_next_replica(main,
						 new_server->server_hash[new_replica_idx],
						 &next_server,
						 &minimum_index);
		if (next_server)
		{
			next_server->handler_replica = minimum_index;
			execute_server_task_queue(next_server);
			dll_node_t *current_data_node = next_server->local_database->head;
			unsigned int server_data_index = 0;
			while (current_data_node)
			{
				dll_node_t *next_data_node =
				dll_get_next_node(next_server->local_database, current_data_node);
				server_data_t *server_data =
				get_server_data_local_database_node(current_data_node);
				/**
				 * store on the new server the documents which are before
				 * the server on the hash ring
				 **/
				if (server_data->associated_replica_index == minimum_index)
				{
					if (new_server->server_hash[new_replica_idx] <
						next_server->server_hash[minimum_index] &&
						(new_server->server_hash[new_replica_idx] > server_data->data_hash ||
						 next_server->server_hash[minimum_index] < server_data->data_hash))
					{
						dll_node_t *rm_node =
						dll_remove_nth_node(next_server->local_database, server_data_index);
						server_data->associated_replica_index = new_replica_idx;
						dll_add_nth_node(new_server->local_database, 0, server_data);

						lru_cache_information cache_key =
						create_lru_cache_information(server_data->name,
													 strlen(server_data->name) + 1);
						lru_cache_remove(next_server->cache, &cache_key);
						free(rm_node->data);
						free(rm_node);
						server_data_index--;
					}
					else if (new_server->server_hash[new_replica_idx] >
							 next_server->server_hash[minimum_index] &&
							 new_server->server_hash[new_replica_idx] > server_data->data_hash &&
							 server_data->data_hash > next_server->server_hash[minimum_index])
					{
						dll_node_t *rm_node =
						dll_remove_nth_node(next_server->local_database, server_data_index);
						server_data->associated_replica_index = new_replica_idx;
						dll_add_nth_node(new_server->local_database, 0, server_data);

						lru_cache_information cache_key =
						create_lru_cache_information(server_data->name,
													 strlen(server_data->name) + 1);
						lru_cache_remove(next_server->cache, &cache_key);
						free(rm_node->data);
						free(rm_node);
						server_data_index--;
					}
					else if (new_server->server_hash[new_replica_idx] ==
							 next_server->server_hash[minimum_index] &&
							 new_server->server_id < next_server->server_id)
					{
						dll_node_t *rm_node = dll_remove_nth_node(next_server->local_database,
																  server_data_index);
						server_data->associated_replica_index = new_replica_idx;
						dll_add_nth_node(new_server->local_database, 0, server_data);

						lru_cache_information cache_key =
						create_lru_cache_information(server_data->name,
													 strlen(server_data->name) + 1);
						lru_cache_remove(next_server->cache, &cache_key);
						free(rm_node->data);
						free(rm_node);
						server_data_index--;
					}
				}
				server_data_index++;
				current_data_node = next_data_node;
			}
		}
	}

	dll_node_t *new_data_node = new_server->local_database->head;
	while (new_data_node)
	{
		server_data_t *server_data =
		get_server_data_local_database_node(new_data_node);
		server_data->associated_replica_index =
		get_associated_label_index_for_data(new_server, server_data);
		new_data_node = dll_get_next_node(new_server->local_database, new_data_node);
	}
	dll_add_nth_node(main->servers, 0, new_server);
	free(new_server);
}

void loader_remove_server(load_balancer *main, int server_id)
{
	unsigned int removing_index = 0;
	dll_node_t *current_server_node = main->servers->head;
	server_t *rm_server = NULL;
	// search of the server with the server ID
	while (current_server_node)
	{
		server_t *current_server = get_server_load_balancer_node(current_server_node);
		if (current_server->server_id == (unsigned int)server_id)
		{
			rm_server = current_server;
			dll_node_t *rm_server_node =
			dll_remove_nth_node(main->servers, removing_index);
			free(rm_server_node);
			break;
		}
		removing_index++;
		current_server_node = dll_get_next_node(main->servers, current_server_node);
	}

	if (!rm_server)
		return;
	rm_server->handler_replica = 0;
	execute_server_task_queue(rm_server);

	// store the data from the removed server on the next server on the hash ring
	dll_node_t *current_data_node = rm_server->local_database->head;
	while (current_data_node && dll_get_size(main->servers) > 0)
	{
		dll_node_t *next_data_node =
		dll_get_next_node(rm_server->local_database, current_data_node);
		server_data_t *server_data =
		get_server_data_local_database_node(current_data_node);
		server_t *next_server = NULL;
		unsigned int minimum_index = 0;
		get_next_replica(main,
						 rm_server->server_hash[server_data->associated_replica_index],
						 &next_server,
						 &minimum_index);
		server_data->associated_replica_index = minimum_index;
		dll_add_nth_node(next_server->local_database, 0, server_data);
		dll_remove_nth_node(rm_server->local_database, 0);
		free(current_data_node->data);
		free(current_data_node);
		current_data_node = next_data_node;
	}

	free_server(&rm_server);
}

response *loader_forward_request(load_balancer *main, request *req)
{
	unsigned int hash = main->hash_function_docs(req->doc_name);
	server_t *server = NULL;
	unsigned int index = 0;
	get_next_replica(main, hash, &server, &index);
	req->replica_index = index;
	server->handler_replica = req->replica_index;
	return server_handle_request(server, req);
}

void free_load_balancer(load_balancer **main)
{
	unsigned int no_servers = dll_get_size((*main)->servers);
	for (unsigned int i = 0; i < no_servers; i++)
	{
		dll_node_t *server_node = dll_remove_nth_node((*main)->servers, 0);
		server_t *server = get_server_load_balancer_node(server_node);

		free_server(&server);
		free(server_node);
	}
	free((*main)->servers);
	free(*main);

	*main = NULL;
}

server_t *get_server_load_balancer_node(dll_node_t *node)
{
	return (server_t *)node->data;
}

void response_free(response *res)
{
	if (res)
	{
		free(res->server_log);
		free(res->server_response);
		free(res);
	}
}

void print_load_balancer(load_balancer *main)
{
	printf("\n--------PRINTING LOAD BALANCER--------\n");
	printf("--------NO. SERVERS - %u--------\n", main->servers->size);
	dll_node_t *server = main->servers->head;
	while (server)
	{
		server_t *s = get_server_load_balancer_node(server);
		print_server(s);
		server = dll_get_next_node(main->servers, server);
	}
	printf("--------END--------\n");
}

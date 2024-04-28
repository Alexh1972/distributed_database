/*
 * Copyright (c) 2024, <>
 */

#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "lru_cache.h"

#include "utils.h"

unsigned int get_server_replica_executor(server_t *s, unsigned int data_hash)
{
	/**
	 * the replica which executes a task associated with the hash
	 * is either the one with the lowest hash greater than the
	 * data hash, if on the hash ring the document isn't the
	 * "last", otherwise its the minimum hash
	*/
	unsigned int global_minimum_hash = -1;
	unsigned int minimum_hash = -1;
	unsigned int minimum_found = 0;
	unsigned int minimum_index = 0;
	unsigned int global_minimum_index = 0;
	for (unsigned int i = 0; i < s->no_replicas; i++)
	{
		if (global_minimum_hash > s->server_hash[i])
		{
			global_minimum_index = i;
			global_minimum_hash = s->server_hash[i];
		}

		if (data_hash < s->server_hash[i])
		{
			if (s->server_hash[i] < minimum_hash)
			{
				minimum_hash = s->server_hash[i];
				minimum_index = i;
				minimum_found = 1;
			}
		}
	}

	if (!minimum_found)
	{
		return global_minimum_index;
	}

	return minimum_index;
}

static response *server_edit_document(server_t *s,
									  char *doc_name,
									  char *doc_content)
{
	unsigned int replica_executor_index =
	get_server_replica_executor(s,
								s->hash_function_docs(doc_name));

	response *res = malloc(sizeof(response));
	res->server_id =
	calculate_replica_label(s->server_id, s->handler_replica);
	lru_cache_information key_info =
	create_lru_cache_information(doc_name, strlen(doc_name) + 1);
	lru_cache_information value_info =
	create_lru_cache_information(doc_content, strlen(doc_content) + 1);
	// search in the cache if document is present
	void *cache_info_document = lru_cache_get(s->cache, &key_info, NULL);
	server_data_t *server_data = get_server_data_by_name(s, doc_name);

	if (server_data)
	{
		// document is already created
		res->server_response = malloc(strlen(MSG_B) - 2 + strlen(doc_name) + 1);
		sprintf(res->server_response, MSG_B, doc_name);

		free(server_data->content);
		server_data->content = malloc(strlen(doc_content) + 1);
		strcpy(server_data->content, doc_content);
	}
	else
	{
		// document is not created
		res->server_response = malloc(strlen(MSG_C) - 2 + strlen(doc_name) + 1);
		sprintf(res->server_response, MSG_C, doc_name);

		server_data_t new_server_data;
		new_server_data.associated_replica_index = replica_executor_index;
		new_server_data.content = malloc(strlen(doc_content) + 1);
		new_server_data.name = malloc(strlen(doc_name) + 1);
		new_server_data.data_hash = s->hash_function_docs(doc_name);
		strcpy(new_server_data.content, doc_content);
		strcpy(new_server_data.name, doc_name);
		dll_add_tail(s->local_database, &new_server_data);
	}

	char *evicted_key = NULL;

	lru_cache_put(s->cache, &key_info, &value_info, (void **)(&evicted_key));

	if (cache_info_document)
	{
		// document was in cache
		res->server_log = malloc(strlen(LOG_HIT) - 2 + strlen(doc_name) + 1);
		sprintf(res->server_log, LOG_HIT, doc_name);
		free(cache_info_document);
	}
	else
	{
		if (!evicted_key)
		{
			// document was not in cache
			res->server_log = malloc(strlen(LOG_MISS) - 2 + strlen(doc_name) + 1);
			sprintf(res->server_log, LOG_MISS, doc_name);
		}
		else
		{
			// a key was evicted from cache
			res->server_log =
			malloc(strlen(LOG_EVICT) - 4 + strlen(doc_name) + strlen(evicted_key) + 1);
			sprintf(res->server_log, LOG_EVICT, doc_name, evicted_key);
			free(evicted_key);
		}
	}

	return res;
}

static response *server_get_document(server_t *s, char *doc_name)
{
	response *res = malloc(sizeof(response));
	res->server_id = calculate_replica_label(s->server_id, s->handler_replica);

	lru_cache_information key_info =
	create_lru_cache_information(doc_name, strlen(doc_name) + 1);
	char *evicted_key = NULL;
	// seach if document is stored in cache
	void *cache_info_document = lru_cache_get(s->cache, &key_info, NULL);
	if (cache_info_document)
	{
		// document was in cache
		res->server_response = strdup(cache_info_document);
		lru_cache_information val_info =
		create_lru_cache_information(res->server_response,
									 strlen(res->server_response) + 1);
		res->server_log = malloc(strlen(LOG_HIT) - 2 + strlen(doc_name) + 1);
		sprintf(res->server_log, LOG_HIT, doc_name);
		free(cache_info_document);
		lru_cache_put(s->cache, &key_info, &val_info, NULL);
	}
	else
	{
		// document is only stored on local database
		server_data_t *server_data = get_server_data_by_name(s, doc_name);
		if (server_data)
		{
			lru_cache_information value_info =
			create_lru_cache_information(server_data->content,
										 strlen(server_data->content) + 1);
			lru_cache_put(s->cache, &key_info, &value_info, (void **)(&evicted_key));
			res->server_response = strdup(server_data->content);
			if (!evicted_key)
			{
				res->server_log = malloc(strlen(LOG_MISS) - 2 + strlen(doc_name) + 1);
				sprintf(res->server_log, LOG_MISS, doc_name);
			}
			else
			{
				res->server_log =
				malloc(strlen(LOG_EVICT) - 4 + strlen(doc_name) + strlen(evicted_key) + 1);
				sprintf(res->server_log, LOG_EVICT, doc_name, evicted_key);
				free(evicted_key);
			}
		}
		else
		{
			res->server_response = NULL;
			res->server_log = malloc(strlen(LOG_FAULT) - 2 + strlen(doc_name) + 1);
			sprintf(res->server_log, LOG_FAULT, doc_name);
		}
	}
	return res;
}

server_t *init_server(unsigned int cache_size,
					  unsigned int server_id,
					  unsigned int (*hash_function_servers)(void *),
					  unsigned int (*hash_function_docs)(void *),
					  unsigned int replicas)
{
	server_t *server = malloc(sizeof(server_t));

	server->cache = init_lru_cache(cache_size);
	server->task_queue = init_queue(sizeof(request));
	server->local_database = dll_create(sizeof(server_data_t));
	server->server_hash = malloc(replicas * sizeof(unsigned int));
	server->no_replicas = replicas;
	server->server_id = server_id;
	server->hash_function_docs = hash_function_docs;
	for (unsigned int i = 0; i < replicas; i++)
	{
		unsigned int label = calculate_replica_label(server->server_id, i);
		server->server_hash[i] = hash_function_servers(&label);
	}
	return server;
}

response *server_handle_request(server_t *s, request *req)
{
	if (req->type == EDIT_DOCUMENT)
	{
		response *res = malloc(sizeof(response));
		res->server_id = calculate_replica_label(s->server_id, req->replica_index);
		res->server_log = malloc((strlen(LOG_LAZY_EXEC) - 2) + MAX_CHAR_SIZE_INT + 1);
		res->server_response =
		malloc((strlen(MSG_A) - 4) + strlen(EDIT_REQUEST) + DOC_NAME_LENGTH + 1);
		request copied_req = copy_request(req);
		push_task_queue(s, &copied_req);
		sprintf(res->server_log, LOG_LAZY_EXEC, get_size_queue(s->task_queue));
		sprintf(res->server_response, MSG_A, EDIT_REQUEST, req->doc_name);
		return res;
	}
	else if (req->type == GET_DOCUMENT)
	{
		execute_server_task_queue(s);
		return server_get_document(s, req->doc_name);
	}

	return NULL;
}

void execute_server_task_queue(server_t *s)
{
	while (!is_empty_queue(s->task_queue))
	{
		request *rqst = peek_queue(s->task_queue);
		response *edit_response =
		server_edit_document(s, rqst->doc_name, rqst->doc_content);
		PRINT_RESPONSE(edit_response);
		ll_node_t *node = pop_queue(s->task_queue);
		request_free((request *)node->data);
		free(node);
	}
}

request copy_request(request *req)
{
	int name_len = strlen(req->doc_name);
	int content_len = strlen(req->doc_content);
	request new_req;
	new_req.doc_content = malloc(content_len + 1);
	new_req.doc_name = malloc(name_len + 1);
	new_req.type = req->type;
	new_req.replica_index = req->replica_index;
	new_req.doc_name[name_len] = 0;
	new_req.doc_content[content_len] = 0;
	memcpy(new_req.doc_content, req->doc_content, content_len);
	memcpy(new_req.doc_name, req->doc_name, name_len);
	return new_req;
}

void free_server(server_t **s)
{
	free_lru_cache(&(*s)->cache);
	while (!is_empty_queue((*s)->task_queue))
	{
		ll_node_t *node = pop_queue((*s)->task_queue);
		request_free((request *)node->data);
		free(node);
	}
	destroy_queue(&((*s)->task_queue));

	while (dll_get_size((*s)->local_database) != 0)
	{
		dll_node_t *rm_node = dll_remove_nth_node((*s)->local_database, 0);
		server_data_t *sd = get_server_data_local_database_node(rm_node);
		server_data_free(sd);
		free(rm_node);
	}
	free((*s)->server_hash);
	dll_free(&((*s)->local_database));
	free(*s);
	*s = NULL;
}

server_data_t *
get_server_data_local_database_node(dll_node_t *local_database_node)
{
	return (server_data_t *)local_database_node->data;
}

void push_task_queue(server_t *s, void *data)
{
	if (get_size_queue(s->task_queue) < TASK_QUEUE_SIZE)
		push_queue(s->task_queue, data);
}

void server_data_free(server_data_t *server_data)
{
	free(server_data->content);
	free(server_data->name);
	free(server_data);
}

void request_free(request *req)
{
	if (req)
	{
		free(req->doc_content);
		free(req->doc_name);
		free(req);
	}
}

void print_server(server_t *server)
{
	printf("\n--------PRINTING SERVER - ID: %u--------\n", server->server_id);
	for (unsigned int i = 0; i < server->no_replicas; i++)
		printf("--------%u. REPLICA: %u - %u - %u--------\n",
			   i,
			   calculate_replica_label(server->server_id, i), server->server_hash[i],
			   number_digits(server->server_hash[i]));
	printf("--------DATA--------\n");
	printf("--------NO. DATA - %u--------\n", server->local_database->size);
	dll_node_t *sd_node = server->local_database->head;
	while (sd_node)
	{
		server_data_t *sd = get_server_data_local_database_node(sd_node);
		print_server_data(sd);
		sd_node = dll_get_next_node(server->local_database, sd_node);
	}
	printf("--------TASK QUEUE SIZE: %u--------\n",
		   get_size_queue(server->task_queue));
	if (!is_empty_queue(server->task_queue))
	{
		request *req = peek_queue(server->task_queue);
		printf("TASK QUEUE TOP KEY: %s -------- VALUE: %s - HASH - %u - %u\n",
			   req->doc_name,
			   req->doc_content,
			   server->hash_function_docs(req->doc_name),
			   number_digits(server->hash_function_docs(req->doc_name)));
	}
	print_lru_cache(server->cache);
}

void print_server_data(server_data_t *server_data)
{
	printf("\n--------NAME - %s - HASH - %u - %u--------\n",
		   server_data->name,
		   server_data->data_hash,
		   number_digits(server_data->data_hash));
	printf("\n--------REPLICA ASSOCIATED - %u--------\n",
		   server_data->associated_replica_index);
}

server_data_t *get_server_data_by_name(server_t *server, char *name)
{
	dll_node_t *server_data_node = server->local_database->head;

	while (server_data_node)
	{
		server_data_t *sd = get_server_data_local_database_node(server_data_node);
		if (strcmp(sd->name, name) == 0)
			return sd;

		server_data_node =
		dll_get_next_node(server->local_database, server_data_node);
	}

	return NULL;
}

unsigned int calculate_replica_label(unsigned int server_id,
									 unsigned int replica_number)
{
	return REPLICA_OFFSET * replica_number + server_id;
}

unsigned int get_associated_label_index_for_data(server_t *server,
												 server_data_t *server_data)
{
	unsigned int minimum_found = 0;
	unsigned int minimum_hash = -1;
	unsigned int minimum_index = 0;
	unsigned int global_minimum = -1;
	unsigned int global_minimum_index = 0;
	for (unsigned int i = 0; i < server->no_replicas; i++)
	{
		if (server->server_hash[i] > server_data->data_hash)
		{
			if (minimum_hash > server->server_hash[i])
			{
				minimum_hash = server->server_hash[i];
				minimum_found = 1;
				minimum_index = i;
			}
		}

		if (global_minimum > server->server_hash[i])
		{
			global_minimum = server->server_hash[i];
			global_minimum_index = i;
		}
	}

	if (!minimum_found)
		return global_minimum_index;
	else
		return minimum_index;
}

/*
 * Copyright (c) 2024, <>
 */

#ifndef SERVER_H
#define SERVER_H

#include "utils.h"
#include "constants.h"
#include "lru_cache.h"
#include "queue.h"
#define TASK_QUEUE_SIZE 1000
#define MAX_LOG_LENGTH 100
#define MAX_RESPONSE_LENGTH 4096
#define REPLICA_OFFSET 100000
#define MAX_REPLICAS 3

typedef struct server
{
    lru_cache *cache;
    queue_t *task_queue;
    doubly_linked_list_t *local_database;
    unsigned int server_id;
    unsigned int *server_hash;
    unsigned int no_replicas;
    unsigned int handler_replica;
    unsigned int (*hash_function_docs)(void *);
} server_t;

typedef struct server_data
{
    char *name;
    char *content;
    unsigned int data_hash;
    unsigned int associated_replica_index;
} server_data_t;

typedef struct request
{
    request_type type;
    unsigned int replica_index;
    char *doc_name;
    char *doc_content;
} request;

typedef struct response
{
    char *server_log;
    char *server_response;
    int server_id;
} response;

server_t *init_server(unsigned int cache_size,
                      unsigned int server_id,
                      unsigned int (*hash_function_servers)(void *),
                      unsigned int (*hash_function_docs)(void *),
                      unsigned int replicas);

/**
 * @brief Should deallocate completely the memory used by server,
 *     taking care of deallocating the elements in the queue, if any,
 *     without executing the tasks
 */
void free_server(server_t **s);

/**
 * server_handle_request() - Receives a request from the load balancer
 *      and processes it according to the request type
 *
 * @param s: Server which processes the request.
 * @param req: Request to be processed.
 *
 * @return response*: Response of the requested operation, which will
 *      then be printed in main.
 *
 * @brief Based on the type of request, should call the appropriate
 *     solver, and should execute the tasks from queue if needed (in
 *     this case, after executing each task, PRINT_RESPONSE should
 *     be called).
 */
response *server_handle_request(server_t *s, request *req);

/**
 * get_server_data_local_database_node() - Gets the data stored in the
 * local database node of the server.
 * 
 * @param local_database_node: The local database node.
 * @returns server_data_t* - Data from the local database node.
*/
server_data_t *
get_server_data_local_database_node(dll_node_t *local_database_node);

void server_data_free(server_data_t *server_data);

/**
 * push_task_queue() - Pushes request to task queue until limit
 * is reached.
 * 
 * @param s: Server on which queue will be executed.
 * @param data: Data pushed to the task queue.
*/
void push_task_queue(server_t *s, void *data);

void request_free(request *req);

void print_server_data(server_data_t *server_data);

void print_server(server_t *server);

/**
 * copy_request() - Copies the reques.
 * 
 * @param req: Request to be copied.
 * @return request - New request which contains
 * the same information with the one sent as
 * parameter.
*/
request copy_request(request *req);

/**
 * get_server_data_by_name() - Gets the data of the document with
 * a specific name.
 * 
 * @param server: Server on which the search will be done.
 * @param name: The name of the document.
 * @return server_data_t* - The data of the document found.
*/
server_data_t *get_server_data_by_name(server_t *server, char *name);

/**
 * execute_server_task_queue() - Executes the whole task queue and
 * empties it.
 * 
 * @param s: Server whose task queue will be executed.
*/
void execute_server_task_queue(server_t *s);

/**
 * calculate_replica_label() - Calculates the ID of the replica
 * of a specific server.
 * 
 * @param server_id: The server ID.
 * @param replica_number: Index of the replica.
 * @return unsigned int - ID of the replica.
*/
unsigned int calculate_replica_label(unsigned int server_id,
                                     unsigned int replica_number);

/**
 * get_server_replica_executor() - Gets the index of the replica which
 * will execute a specific task.
 * 
 * @param s: The server which will execute the task.
 * @param data_hash: The hash of the data on which the task will be performed.
 * @return unsigned int - The index of the replica.
*/
unsigned int get_server_replica_executor(server_t *s, unsigned int data_hash);

/**
 * get_associated_label_index_for_data() - Gets the index of the replica which
 * is associated with a specific server document.
 * 
 * @param server: The server on which search will be performed..
 * @param server_data: The server document.
 * @return unsigned int - The index of the replica.
*/
unsigned int get_associated_label_index_for_data(server_t *server,
                                                 server_data_t *server_data);

#endif /* SERVER_H */

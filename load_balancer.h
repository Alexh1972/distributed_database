/*
 * Copyright (c) 2024, <>
 */

#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include "server.h"
#include "linked_list.h"

#define MAX_SERVERS             99999

typedef struct load_balancer {
    unsigned int (*hash_function_servers)(void *);
    unsigned int (*hash_function_docs)(void *);
    bool enabled_vnodes;
    doubly_linked_list_t *servers;
} load_balancer;


load_balancer *init_load_balancer(bool enable_vnodes);

void free_load_balancer(load_balancer** main);

/**
 * loader_add_server() - Adds a new server to the system.
 * 
 * @param main: Load balancer which distributes the work.
 * @param server_id: ID of the new server.
 * @param cache_size: Capacity of the new server's cache.
 * 
 * @brief The load balancer will generate 1 or 3 replica labels and will place
 * them inside the hash ring. The neighbor servers will distribute SOME of the
 * documents to the added server. Before distributing the documents, these
 * servers should execute all the tasks in their queues.
 */
void loader_add_server(load_balancer* main, int server_id, int cache_size);

/**
 * loader_remove_server() Removes a server from the system.
 * 
 * @param main: Load balancer which distributes the work.
 * @param server_id: ID of the server to be removed.
 * 
 * @brief The load balancer will remove the server (and its replicas) from
 * the hash ring and will distribute ALL documents stored on the removed
 * server to the "neighboring" servers.
 * 
 * Additionally, all the tasks stored in the removed server's queue
 * should be executed before moving the documents.
 */
void loader_remove_server(load_balancer* main, int server_id);

/**
 * loader_forward_request() - Forwards a request to the appropriate server.
 * 
 * @param main: Load balancer which distributes the work.
 * @param req: Request to be forwarded (relevant fields from the request are
 *        dynamically allocated, but the caller have to free them).
 * 
 * @return response* - Contains the response received from the server
 * 
 * @brief The load balancer will find the server which should handle the
 * request and will send the request to that server. The request will contain
 * the document name and/or content, which are dynamically allocated in main
 * and should be freed either here, either in server_handle_request, after
 * using them.
 */
response *loader_forward_request(load_balancer* main, request *req);

/**
 * get_server_load_balancer_node() - Gets the server from a load
 * balancer list node.
 * 
 * @param node: Load balacer list node.
 * @return server_t* - The server of the specific node.
*/
server_t *get_server_load_balancer_node(dll_node_t *node);

void response_free(response *res);

void print_load_balancer(load_balancer *main);

/**
 * get_number_replicas() - Gets the number of replicas set to be used.
 * 
 * @param main: The load balancer.
 * @return: unsigned int - The number of replicas used by the load balancer.
*/
unsigned int get_number_replicas(load_balancer *main);

/**
 * get_next_replica() - For a specific hash, it finds the next server on the
 * hash ring.
 * 
 * @param main: The load balancer.
 * @param hash: The hash for which the next server on the hash ring is
 * searched.
 * @param server: Parameter through which the caller retrieves the address
 * of the next server. If caller sets it to NULL, no server will be returned,
 * otherwise it will contain a pointer to the specific server.
 * @param index: Parameter through which the caller retrieves the replica
 * index associated to the node which is placed after the hash on the hash
 * ring. If caller sets it to NULL, no server will be returned, otherwise
 * it will cointain the index of the specific replica.
*/
void get_next_replica(load_balancer *main,
                      unsigned int hash,
                      server_t **server,
                      unsigned int *index);

#endif /* LOAD_BALANCER_H */

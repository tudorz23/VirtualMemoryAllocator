/* Copyright Marius-Tudor Zaharia 313CAa 2022-2023 */
#ifndef __DOUBLY_LIST_H
#define __DOUBLY_LIST_H

#include "utils.h"

typedef struct node_t {
	struct node_t *prev;
	struct node_t *next;
	void *data;
} node_t;

typedef struct {
	node_t *head;
	unsigned int size;
	size_t data_size;
} list_t;

list_t *list_create(size_t data_size);

void add_nth_node(list_t *list, unsigned int n, void *new_data);
node_t *remove_nth_node(list_t *list, unsigned int n);

node_t *get_nth_node(list_t *list, unsigned int n);
void list_free(list_t **list);

void miniblock_list_free(list_t **list);
void destroy_block_from_list(node_t *block_node);
void free_miniblock(node_t *miniblock_to_free);

#endif /* __DOUBLY_LIST_H_ */

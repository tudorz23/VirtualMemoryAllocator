/* Copyright Marius-Tudor Zaharia 313CAa 2022-2023 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "doubly_list.h"
#include "vma.h"
#include "utils.h"

// Creeaza si initializeaza o lista dublu inlantuita
list_t *list_create(size_t data_size)
{
	list_t *list = malloc(sizeof(*list));
	DIE(!list, "list alloc failed\n");

	list->head = NULL;
	list->size = 0;
	list->data_size = data_size;

	return list;
}

// Creeaza si adauga un nou nod pe pozitia n, in lista list,
// copiind new_data informatie.
// Functioneaza si pentru prima sau ultima pozitie.
void add_nth_node(list_t *list, unsigned int n, void *new_data)
{
	// daca lista nu este alocata
	if (!list)
		return;

	if (n >= list->size)
		n = list->size;

	node_t *new_node = malloc(sizeof(*new_node));
	DIE(!new_node, "new_node alloc failed\n");

	new_node->next = NULL;
	new_node->prev = NULL;

	new_node->data = malloc(list->data_size);
	DIE(!new_node->data, "data alloc failed\n");

	memcpy(new_node->data, new_data, list->data_size);

	// parcurgem lista pana la pozitia n
	node_t *curr = list->head;

	// daca adaug pe ultima pozitie;
	// trebuie parcurs diferit, altfel ajung
	// sa dereferentiez NULL prin curr->next
	if (n == list->size && list->size != 0) {
		// vreau sa detin ultimul nod in curr
		while (curr->next)
			curr = curr->next;

		curr->next = new_node;
		new_node->prev = curr;

		list->size++;
		return;
	}

	int copy = n;
	while (n) {
		curr = curr->next;
		n--;
	}

	// daca lista este goala
	if (list->size == 0) {
		list->head = new_node;
	} else if (copy == 0) {
		// daca adaug pe pozitia 0, dar lista nu este goala
		list->head = new_node;
		curr->prev = new_node;
		new_node->next = curr;
	} else {
		curr->prev->next = new_node;
		new_node->prev = curr->prev;
		new_node->next = curr;
		curr->prev = new_node;
	}

	list->size++;
}

// Elimina din lista nodul de pe pozitia n si intoarce pointer catre acesta,
// memoria urmand sa fie eliberata de apelant prin intermediul acestui pointer.
node_t *remove_nth_node(list_t *list, unsigned int n)
{
	// daca nu exista lista sau daca este goala
	if (!list || !list->head)
		return NULL;

	if (n >= list->size)
		n = list->size - 1;

	node_t *curr = list->head;

	// daca lista are un singur element si il elimin
	if (list->size == 1) {
		list->head = NULL;
		list->size--;
		return curr;
	}

	int copy = n;
	while (n) {
		curr = curr->next;
		n--;
	}

	// daca elimin nodul de pe pozitia 0
	if (copy == 0) {
		list->head = curr->next;
		curr->next->prev = NULL;
	} else if (!curr->next) {
		// daca elimin nodul de pe ultima pozitie
		curr->prev->next = NULL;
	} else {
		curr->prev->next = curr->next;
		curr->next->prev = curr->prev;
	}

	list->size--;
	return curr;
}

// Returneaza pointer catre nodul de pe pozitia n din lista
node_t *get_nth_node(list_t *list, unsigned int n)
{
	// daca nu exista lista
	if (!list)
		return NULL;

	if (n >= list->size)
		n = list->size - 1;

	node_t *curr = list->head;

	while (n) {
		curr = curr->next;
		n--;
	}

	return curr;
}

// Elibereaza memoria folosita de fiecare nod din lista,
// iar la final elibereaza memoria ocupata de lista
void list_free(list_t **list)
{
	if (!(*list))
		return;

	if (!(*list)->head) {
		free(*list);
		*list = NULL;
		return;
	}

	node_t *curr = (*list)->head;
	node_t *aux = NULL;

	while ((*list)->size != 0) {
		aux = curr->next;
		free(curr->data);
		free(curr);

		curr = aux;
		(*list)->head = aux;
		(*list)->size--;
	}

	free(*list);
	*list = NULL;
}

// Ma scapa de o parcurgere suplimentara a listei de miniblocuri
// Elibereaza direct si rw_buffer, daca exista, spre deosebire
// de list_free
void miniblock_list_free(list_t **list)
{
	if (!(*list))
		return;

	if (!(*list)->head) {
		free(*list);
		*list = NULL;
		return;
	}

	node_t *curr = (*list)->head;
	node_t *aux = NULL;

	while ((*list)->size != 0) {
		aux = curr->next;
		miniblock_t *info = (miniblock_t *)(curr->data);
		if (info->rw_buffer) {
			free(info->rw_buffer);
			info->rw_buffer = NULL;
		}
		free(curr->data);
		free(curr);

		curr = aux;
		(*list)->head = aux;
		(*list)->size--;
	}

	free(*list);
	*list = NULL;
}

// Elibereaza memoria unui block intreg
// Utila pentru a scrie mai putine linii
// Gandita pentru a fi folosita dupa un apel al functiei remove_nth_node
void destroy_block_from_list(node_t *block_node)
{
	block_t *block = (block_t *)(block_node->data);
	miniblock_list_free(&block->miniblock_list);

	free(block_node->data);
	free(block_node);
}

// Elibereaza memoria unui miniblock
// La fel, economiseste linii
void free_miniblock(node_t *miniblock_to_free)
{
	miniblock_t *info = (miniblock_t *)(miniblock_to_free->data);

	if (info->rw_buffer) {
		free(info->rw_buffer);
		info->rw_buffer = NULL;
	}

	free(miniblock_to_free->data);
	free(miniblock_to_free);
}

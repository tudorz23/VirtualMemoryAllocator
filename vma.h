/* Copyright Marius-Tudor Zaharia 313CAa 2022-2023 */
#ifndef __VMA_H_
#define __VMA_H_

#include <inttypes.h>
#include <stddef.h>

#include "doubly_list.h"

#define MAX_INPUT 1000

typedef struct {
	uint64_t start_address;
	size_t size;
	list_t *miniblock_list;
} block_t;

typedef struct {
	uint64_t start_address;
	size_t size;
	uint8_t perm;
	void *rw_buffer;
} miniblock_t;

typedef struct {
	uint64_t arena_size;
	// pentru PMAP
	uint64_t free_space;
	int block_number;
	int miniblock_number;
	list_t *alloc_list;
} arena_t;

void treat_error(char *user_input);

void parse_alloc_arena(char *user_input, uint64_t *value, int *is_valid);

arena_t *alloc_arena(const uint64_t size);
void dealloc_arena(arena_t *arena);

void parse_alloc_block(char *user_input, uint64_t *address, uint64_t *size,
					   int *is_valid);
int block_validity(arena_t *arena, const uint64_t address, const uint64_t size);
int get_position(arena_t *arena, const uint64_t address,
				 unsigned long stop_address, int *left, int *right);
void copy_block(block_t *new_block, node_t *source_block_node, int where);
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);

void parse_free_block(char *user_input, uint64_t *address, int *is_valid);
int free_validity_block(arena_t *arena, const uint64_t address);
void free_block(arena_t *arena, const uint64_t address);

block_t *read_validity(arena_t *arena, const uint64_t address);
void parse_read(arena_t *arena, char *user_input, uint64_t *address,
				uint64_t *size);
void read(arena_t *arena, uint64_t address, uint64_t size);

void parse_write(arena_t *arena, char *user_input, uint64_t *address,
				 uint64_t *size);
block_t *write_validity(arena_t *arena, const uint64_t address);
int8_t *get_write_params(const uint64_t size, int8_t *data);
void write(arena_t *arena, const uint64_t address,  const uint64_t size,
		   int8_t *data);

void parse_pmap(char *user_input, int *is_valid);
void pmap(const arena_t *arena);

int miniblock_read_perm(miniblock_t *miniblock);
int check_read_perm(node_t *miniblock_node, const uint64_t address,
					const uint64_t size);
int miniblock_write_perm(miniblock_t *miniblock);
int check_write_perm(node_t *miniblock_node, const uint64_t address,
					 const uint64_t size);
void parse_mprotect(arena_t *arena, char *user_input, uint64_t *address);
void print_permission(miniblock_t *miniblock);
void mprotect(arena_t *arena, uint64_t address, int8_t *permission);

#endif /* __VMA_H_ */

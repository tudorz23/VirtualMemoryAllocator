/* Copyright Marius-Tudor Zaharia 313CAa 2022-2023 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "vma.h"
#include "doubly_list.h"
#include "utils.h"

int main(void)
{
	arena_t *arena = NULL;
	int is_valid; // ne va da validitatea comenzii introduse
	uint64_t value1, value2; // valori utile pentru parsarea input-urilor
	while (1) {
		char user_input[MAX_INPUT];

		fgets(user_input, MAX_INPUT, stdin);

		// facem o copie pentru a putea "strica" sirul cu strtok
		char copy_of_input[MAX_INPUT];
		memcpy(copy_of_input, user_input, strlen(user_input) + 1);

		// determinam comanda introdusa
		char *command = strtok(copy_of_input, "\n ");

		if (strcmp(command, "ALLOC_ARENA") == 0) {
			parse_alloc_arena(user_input, &value1, &is_valid);
			if (is_valid)
				arena = alloc_arena(value1);

		} else if (strcmp(command, "DEALLOC_ARENA") == 0) {
			dealloc_arena(arena);
			break;

		} else if (strcmp(command, "ALLOC_BLOCK") == 0) {
			parse_alloc_block(user_input, &value1, &value2, &is_valid);
			if (is_valid)
				alloc_block(arena, value1, value2);

		} else if (strcmp(command, "FREE_BLOCK") == 0) {
			parse_free_block(user_input, &value1, &is_valid);
			if (is_valid)
				free_block(arena, value1);

		} else if (strcmp(command, "READ") == 0) {
			parse_read(arena, user_input, &value1, &value2);

		} else if (strcmp(command, "WRITE") == 0) {
			parse_write(arena, user_input, &value1, &value2);

		} else if (strcmp(command, "PMAP") == 0) {
			parse_pmap(user_input, &is_valid);
			if (is_valid)
				pmap(arena);

		} else if (strcmp(command, "MPROTECT") == 0) {
			parse_mprotect(arena, user_input, &value1);

		} else {
			treat_error(user_input);
		}
	}

	return 0;
}

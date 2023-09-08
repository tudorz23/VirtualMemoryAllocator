/* Copyright Marius-Tudor Zaharia 313CAa 2022-2023 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "vma.h"

// afiseaza mesajul de eroare de atatea ori cati parametri sunt dati comenzii
void treat_error(char *user_input)
{
	char *token = strtok(user_input, "\n ");
	while (token) {
		printf("Invalid command. Please try again.\n");
		token = strtok(NULL, "\n ");
	}
}

// Parseaza inputul de la user in cazul in care primul
// cuvant este ALLOC_ARENA si verifica validitatea comenzii
void parse_alloc_arena(char *user_input, uint64_t *value, int *is_valid)
{
	char copy_of_input[MAX_INPUT];
	memcpy(copy_of_input, user_input, strlen(user_input) + 1);

	*is_valid = 1;
	strtok(user_input, "\n "); // sar peste "ALLOC_ARENA"
	char *token = strtok(NULL, "\n ");
	if (!token) {
		*is_valid = 0;
		treat_error(copy_of_input);
		return;
	}

	// verific daca al doilea parametru este numar si daca nu este negativ
	long dummy = atol(token);
	if ((dummy == 0 && strcmp(token, "0") != 0) || dummy < 0) {
		treat_error(copy_of_input);
		*is_valid = 0;
		return;
	}

	// verific daca mai exista si alt parametru
	token = strtok(NULL, "\n ");
	if (token) {
		*is_valid = 0;
		treat_error(copy_of_input);
		return;
	}

	*value = dummy;
}

// Initializeaza arena
arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena = malloc(sizeof(*arena));
	DIE(!arena, "alloc of arena failed\n");

	arena->arena_size = size;
	arena->free_space = size;

	arena->block_number = 0;
	arena->miniblock_number = 0;

	// initializez lista de blocuri (lista principala)
	arena->alloc_list = list_create(sizeof(block_t));

	return arena;
}

// Dealoca toate resursele
void dealloc_arena(arena_t *arena)
{
	node_t *curr = arena->alloc_list->head;

	while (curr) {
		// eliberez miniblocurile fiecarui bloc
		// folosesc aceasta functie pentru a elibera direct si rw_buffer
		// (daca este alocat), economisind o parcurgere
		block_t *curr_block = (block_t *)(curr->data);
		miniblock_list_free(&curr_block->miniblock_list);
		curr = curr->next;
	}

	// eliberez lista de blocuri
	list_free(&arena->alloc_list);
	free(arena);
}

// Parseaza inputul de la user in cazul in care primul
// cuvant este ALLOC_BLOCK si verifica validitatea comenzii
void parse_alloc_block(char *user_input, uint64_t *address, uint64_t *size,
					   int *is_valid)
{
	char copy_of_input[MAX_INPUT];
	memcpy(copy_of_input, user_input, strlen(user_input) + 1);

	*is_valid = 1;
	strtok(user_input, "\n "); // sar peste "ALLOC_BLOCK"

	char *token = strtok(NULL, "\n ");
	if (!token) {
		*is_valid = 0;
		treat_error(copy_of_input);
		return;
	}

	// verific daca al doilea parametru este numar si daca nu este negativ
	long dummy = atol(token);
	if ((dummy == 0 && strcmp(token, "0") != 0) || dummy < 0) {
		treat_error(copy_of_input);
		*is_valid = 0;
		return;
	}

	*address = dummy;

	token = strtok(NULL, "\n ");
	if (!token) {
		*is_valid = 0;
		treat_error(copy_of_input);
		return;
	}

	// verific al treilea parametru
	dummy = atol(token);
	if ((dummy == 0 && strcmp(token, "0") != 0) || dummy < 0) {
		treat_error(copy_of_input);
		*is_valid = 0;
		return;
	}

	*size = dummy;

	// verific daca mai exista si alt parametru
	token = strtok(NULL, "\n ");
	if (token) {
		*is_valid = 0;
		treat_error(copy_of_input);
		return;
	}
}

// Verifica daca noul bloc este valid (daca nu intersecteaza alte blocuri)
int block_validity(arena_t *arena, const uint64_t address, const uint64_t size)
{
	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return 0;
	}

	unsigned long stop_address = address + size - 1;
	if (stop_address >= arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return 0;
	}

	// parcurg lista de blocuri
	node_t *curr = arena->alloc_list->head;
	while (curr) {
		block_t *curr_block = (block_t *)(curr->data);
		// inceput de bloc
		unsigned long block_start = curr_block->start_address;
		// sfarsit de bloc
		unsigned long block_end = block_start + curr_block->size - 1;

		if ((address >= block_start && address <= block_end) ||
			(stop_address >= block_start && stop_address <= block_end) ||
			(address < block_start && stop_address > block_end)) {
			printf("This zone was already allocated.\n");
			return 0;
		}
		curr = curr->next;
	}
	return 1;
}

// Returneaza pozitia in lista de blocuri pe care trebuie inserat noul nod
// *left = 1 daca exista block adiacent la stanga lui, 0 altfel
// *right = 1 daca exista block adiacent la dreapta lui, 0 altfel
int get_position(arena_t *arena, const uint64_t address,
				 unsigned long stop_address, int *left, int *right)
{
	// daca lista de blocuri este goala, inserez pe pozitia 0
	if (arena->alloc_list->size == 0)
		return 0;

	// curr porneste drept nodul de pe pozitia 0
	node_t *curr = arena->alloc_list->head;

	// accesez data, care era generica, de tip void*
	block_t *curr_block = (block_t *)(curr->data);

	// verific daca trebuie sa inserez pe pozitia 0
	if (stop_address < curr_block->start_address) {
		// vad daca blocurile sunt adiacente
		if (stop_address + 1 == curr_block->start_address)
			*right = 1;
		return 0;
	}

	int position = 0;

	node_t *aux = curr->next;
	block_t *aux_block;

	// parcurg lista de blocuri, incercand sa
	// incadrez noul bloc intre alte 2 blocuri
	while (aux) {
		position++;

		curr_block = (block_t *)(curr->data);
		aux_block = (block_t *)(aux->data);

		// sfarsitul blocului din stanga
		unsigned long left_end = curr_block->start_address +
								 curr_block->size - 1;

		// inceputul blocului din dreapta
		unsigned long right_start = aux_block->start_address;

		// daca am reusit sa incadrez noul bloc intre 2 blocuri
		if (left_end < address && stop_address < right_start) {
			// verific daca blocurile sunt adiacente
			if (left_end + 1 == address)
				*left = 1;

			if (stop_address + 1 == right_start)
				*right = 1;

			return position;
		}

		curr = curr->next;
		aux = aux->next;
	}

	// daca am ajuns aici, sigur trebuie sa inserez la finalul listei
	// verific adiacenta (posibil la stanga)
	curr_block = (block_t *)(curr->data); // ultimul bloc
	unsigned long left_end = curr_block->start_address + curr_block->size - 1;

	if (left_end + 1 == address)
		*left = 1;

	return arena->alloc_list->size;
}

// Copiaza toate miniblocurile dintr-un anumit bloc in alt bloc
// Utila pentru concatenarea blocurilor
// position reprezinta pozitia unde incepe sa adauge miniblocurile noi
void copy_block(block_t *new_block, node_t *source_block_node, int position)
{
	// blocul efectiv
	block_t *source_block = (block_t *)(source_block_node->data);

	// parcurg blocul si copiez miniblocurile in noul bloc
	node_t *curr = source_block->miniblock_list->head;
	while (curr) {
		miniblock_t *source_miniblock = (miniblock_t *)(curr->data);

		miniblock_t to_add_miniblock;
		to_add_miniblock.start_address = source_miniblock->start_address;
		to_add_miniblock.size = source_miniblock->size;
		to_add_miniblock.perm = source_miniblock->perm;

		if (!source_miniblock->rw_buffer) {
			to_add_miniblock.rw_buffer = NULL;
		} else {
			to_add_miniblock.rw_buffer = malloc(source_miniblock->size);
			DIE(!to_add_miniblock.rw_buffer, "malloc failed\n");

			memcpy(to_add_miniblock.rw_buffer, source_miniblock->rw_buffer,
				   source_miniblock->size);
		}

		add_nth_node(new_block->miniblock_list, position, &to_add_miniblock);
		position++;
		curr = curr->next;
	}
	new_block->size += source_block->size;
}

// Adauga un nou miniblock in arena
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	int valid = block_validity(arena, address, size);
	if (!valid)
		return;

	// ne vor spune daca blocul nou are vecini adiacenti
	int left = 0, right = 0;
	unsigned long stop_address = address + size - 1;

	// pozitia unde ar trebui adaugat noul bloc
	int position = get_position(arena, address, stop_address, &left, &right);

	miniblock_t new_miniblock;
	new_miniblock.start_address = address;
	new_miniblock.size = size;
	new_miniblock.perm = 6;
	new_miniblock.rw_buffer = NULL;

	arena->miniblock_number++; // se adauga un minibloc
	arena->free_space -= size;

	if (left == 0 && right == 0) {
		// creez noul bloc
		block_t new_block;
		new_block.start_address = address;
		new_block.size = size;
		new_block.miniblock_list = list_create(sizeof(miniblock_t));

		// adaug miniblock-ul in lista de miniblocuri a blocului
		add_nth_node(new_block.miniblock_list, 0, &new_miniblock);

		// adaug blocul in lista de blocuri
		add_nth_node(arena->alloc_list, position, &new_block);

		arena->block_number++; // se adauga un bloc

	} else if (left == 1 && right == 0) {
		// voi adauga miniblocul la finalul listei de miniblocuri
		// a blocului din stanga
		node_t *node_left_block = get_nth_node(arena->alloc_list, position - 1);
		block_t *left_block = (block_t *)(node_left_block->data);
		add_nth_node(left_block->miniblock_list,
					 left_block->miniblock_list->size, &new_miniblock);

		left_block->size += new_miniblock.size;

	} else if (right == 1 && left == 0) {
		// voi adauga miniblocul la inceputul listei de miniblocuri
		// a blocului din dreapta
		node_t *node_right_block = get_nth_node(arena->alloc_list, position);
		block_t *right_block = (block_t *)(node_right_block->data);
		right_block->start_address = address; // actualizez adresa de start
		add_nth_node(right_block->miniblock_list, 0, &new_miniblock);

		right_block->size += new_miniblock.size;

	} else {
		// voi adauga miniblocul la finalul listei de miniblocuri
		// a blocului din stanga
		node_t *node_left_block = get_nth_node(arena->alloc_list, position - 1);
		block_t *left_block = (block_t *)(node_left_block->data);
		left_block->size += size;
		add_nth_node(left_block->miniblock_list,
					 left_block->miniblock_list->size, &new_miniblock);

		node_t *node_right_block = remove_nth_node(arena->alloc_list,
												   position);
		// copiez blocul din dreapta la finalul listei de miniblocuri
		// a blocului din stanga
		copy_block(left_block, node_right_block,
				   left_block->miniblock_list->size);

		// eliberez blocul eliminat
		destroy_block_from_list(node_right_block);

		arena->block_number--; // se pierde un bloc prin unirea celor 3
	}
}

// Parseaza inputul de la user in cazul in care primul
// cuvant este FREE_BLOCK si verifica validitatea comenzii
void parse_free_block(char *user_input, uint64_t *address, int *is_valid)
{
	char copy_of_input[MAX_INPUT];
	memcpy(copy_of_input, user_input, strlen(user_input) + 1);

	*is_valid = 1;
	strtok(user_input, "\n "); // sar peste "FREE_BLOCK"

	char *token = strtok(NULL, "\n ");
	if (!token) {
		*is_valid = 0;
		treat_error(copy_of_input);
		return;
	}

	// verific daca al doilea parametru este numar si daca nu este negativ
	long dummy = atol(token);
	if ((dummy == 0 && strcmp(token, "0") != 0) || dummy < 0) {
		treat_error(copy_of_input);
		*is_valid = 0;
		return;
	}

	*address = dummy;

	// verific daca mai exista si alt parametru
	token = strtok(NULL, "\n ");
	if (token) {
		*is_valid = 0;
		treat_error(copy_of_input);
		return;
	}
}

// Verifica daca zona de free este valida
// Returneaza -1 daca nu este valida sau
// pozitia blocului in care se afla daca e posibil sa fie acolo
// Ramane de vazut daca este adresa de inceput a unui minibloc
int free_validity_block(arena_t *arena, const uint64_t address)
{
	if (address >= arena->arena_size) {
		printf("Invalid address for free.\n");
		return -1;
	}

	// parcurg lista de blocuri
	node_t *curr = arena->alloc_list->head;
	int position = 0;
	while (curr) {
		block_t *curr_block = (block_t *)(curr->data);
		// inceput de bloc
		unsigned long block_start = curr_block->start_address;
		// sfarsit de bloc
		unsigned long block_end = block_start + curr_block->size - 1;

		if (address >= block_start && address <= block_end)
			return position;

		curr = curr->next;
		position++;
	}

	// Daca ajung aici, inseamna ca zona de memorie nu este alocata
	printf("Invalid address for free.\n");
	return -1;
}

// Elibereaza un miniblock
void free_block(arena_t *arena, const uint64_t address)
{
	int position = free_validity_block(arena, address);
	if (position < 0)
		return;
	node_t *to_free_block_node = get_nth_node(arena->alloc_list, position);
	block_t *to_free_block = (block_t *)(to_free_block_node->data);

	int cnt = 0;
	node_t *curr = to_free_block->miniblock_list->head;
	while (curr) {
		cnt++;
		miniblock_t *to_free_miniblock = (miniblock_t *)(curr->data);
		if (to_free_miniblock->start_address == address &&
			to_free_miniblock->size == to_free_block->size &&
			to_free_block->start_address == address) {
			// este singurul minibloc din bloc, deci elimin blocul
			arena->free_space += to_free_block->size;
			arena->block_number--;
			arena->miniblock_number--;

			remove_nth_node(arena->alloc_list, position);
			destroy_block_from_list(to_free_block_node);
			return;
		}
		if (to_free_miniblock->start_address == address && curr->next &&
			curr->prev) {
			// e in mijlocul blocului, deci creez bloc nou la dreapta sa
			arena->block_number++;
			arena->free_space += to_free_miniblock->size;

			block_t new_block;
			new_block.miniblock_list = list_create(sizeof(miniblock_t));
			new_block.start_address = address + to_free_miniblock->size;
			new_block.size = 0;

			copy_block(&new_block, to_free_block_node, 0);

			for (int i = 0; i < cnt; i++) {
				node_t *aux = remove_nth_node(new_block.miniblock_list, 0);

				new_block.size -= ((miniblock_t *)(aux->data))->size;
				free_miniblock(aux);
			}
			add_nth_node(arena->alloc_list, position + 1, &new_block);

			cnt--;
			curr = curr->prev;
			while (curr->next) {
				node_t *curr_miniblock =
				remove_nth_node(to_free_block->miniblock_list, cnt);
				miniblock_t *info = (miniblock_t *)(curr_miniblock->data);

				to_free_block->size -= info->size;
				free_miniblock(curr_miniblock);
			}
			arena->miniblock_number--;
			return;
		}
		if (to_free_miniblock->start_address == address) {
			// e la inceput sau la sfarsit de bloc
			arena->free_space += to_free_miniblock->size;
			cnt--;
			remove_nth_node(to_free_block->miniblock_list, cnt);

			to_free_block->size -= to_free_miniblock->size;

			// daca este primul minibloc din bloc
			if (to_free_block->start_address ==
				to_free_miniblock->start_address)
				to_free_block->start_address =
				to_free_miniblock->start_address + to_free_miniblock->size;

			free_miniblock(curr);
			arena->miniblock_number--;
			return;
		}
		curr = curr->next;
	}
	printf("Invalid address for free.\n");
}

// Parseaza inputul de la user in cazul in care primul
// cuvant este READ si verifica validitatea comenzii
void parse_read(arena_t *arena, char *user_input, uint64_t *address,
				uint64_t *size)
{
	char copy_of_input[MAX_INPUT];
	memcpy(copy_of_input, user_input, strlen(user_input) + 1);

	strtok(user_input, "\n "); // sar peste "READ"

	char *token = strtok(NULL, "\n ");
	if (!token) {
		treat_error(copy_of_input);
		return;
	}

	// verific daca al doilea parametru este numar si daca nu este negativ
	long dummy = atol(token);
	if ((dummy == 0 && strcmp(token, "0") != 0) || dummy < 0) {
		treat_error(copy_of_input);
		return;
	}

	*address = dummy;

	token = strtok(NULL, "\n ");
	if (!token) {
		treat_error(copy_of_input);
		return;
	}

	// verific al treilea parametru
	dummy = atol(token);
	if ((dummy == 0 && strcmp(token, "0") != 0) || dummy < 0) {
		treat_error(copy_of_input);
		return;
	}

	*size = dummy;

	// verific daca mai exista si alt parametru
	token = strtok(NULL, "\n ");
	if (token) {
		treat_error(copy_of_input);
		return;
	}

	// daca am ajuns aici, comanda de read este valida
	read(arena, *address, *size);
}

// Verifica daca adresa este una valida pentru a citi din ea
block_t *read_validity(arena_t *arena, const uint64_t address)
{
	if (address >= arena->arena_size) {
		printf("Invalid address for read.\n");
		return NULL;
	}

	// parcurg lista de blocuri
	node_t *curr = arena->alloc_list->head;
	int position = 0;
	while (curr) {
		block_t *curr_block = (block_t *)(curr->data);
		// inceput de bloc
		unsigned long block_start = curr_block->start_address;
		// sfarsit de bloc
		unsigned long block_end = block_start + curr_block->size - 1;

		if (address >= block_start && address <= block_end)
			return curr_block;

		curr = curr->next;
		position++;
	}

	// Daca ajung aici, inseamna ca zona de memorie nu este alocata
	printf("Invalid address for read.\n");
	return NULL;
}

// Verifica daca miniblocul are permisiuni de citire
int miniblock_read_perm(miniblock_t *miniblock)
{
	if (miniblock->perm >= 4)
		return 1;

	printf("Invalid permissions for read.\n");
	return 0;
}

// Verifica daca vreunul din miniblocurile din care trebuie sa citesc
// nu are permisiuni de citire
int check_read_perm(node_t *miniblock_node, const uint64_t address,
					const uint64_t size)
{
	miniblock_t *miniblock = (miniblock_t *)(miniblock_node->data);
	if (miniblock_read_perm(miniblock) == 0)
		return 0;

	// inceput de minibloc
	unsigned long mini_start = miniblock->start_address;
	// sfarsit de minibloc
	unsigned long mini_end = mini_start + miniblock->size - 1;

	// cat mai am de citit
	unsigned long remained = size;
	remained -= mini_end - address + 1; // scad miniblocul actual

	node_t *aux = miniblock_node->next;
	while (aux && remained) {
		miniblock_t *aux_mini = (miniblock_t *)(aux->data);
		if (remained >= aux_mini->size)
			remained -= aux_mini->size;
		else
			remained = 0;
		if (miniblock_read_perm(aux_mini) == 0)
			return 0;
		aux = aux->next;
	}
	return 1;
}

// Citeste <size> informatie de la adresa <address>
void read(arena_t *arena, uint64_t address, uint64_t size)
{
	block_t *to_read_block = read_validity(arena, address);
	if (!to_read_block)
		return;

	// cat pot citi din acel bloc, incepand cu acea adresa
	unsigned long space = to_read_block->start_address + to_read_block->size
						- address;

	node_t *curr = to_read_block->miniblock_list->head;
	while (curr) {
		miniblock_t *curr_miniblock = (miniblock_t *)(curr->data);
		// inceput de minibloc
		unsigned long mini_start = curr_miniblock->start_address;
		// sfarsit de minibloc
		unsigned long mini_end = mini_start + curr_miniblock->size - 1;

		if (address >= mini_start && address <= mini_end) {
			// verific intai permisiunile pentru fiecare minibloc
			if (check_read_perm(curr, address, size) == 0)
				return;

			unsigned long remained; // cat mai am de citit

			if (space < size) {
				remained = space;
				printf("Warning: size was bigger than the block size. ");
				printf("Reading %lu characters.\n", space);
			} else {
				remained = size;
			}

			unsigned long to_read = 0; // dimensiunea lui rw_buffer ce tb citita

			if (remained >= (mini_end - address + 1))
				to_read = mini_end - address + 1;
			else
				to_read = remained;

			unsigned long start_read = address - mini_start;
			unsigned long stop_read = start_read + to_read;
			for (unsigned long i = start_read; i < stop_read; i++) {
				printf("%c", ((int8_t *)(curr_miniblock->rw_buffer))[i]);
				remained--;
			}
			node_t *aux = curr->next;
			while (remained && aux) {
				miniblock_t *aux_mini = (miniblock_t *)(aux->data);

				to_read = 0; // dimensiunea lui rw_buffer ce tb citita

				if (remained >= aux_mini->size)
					to_read = aux_mini->size;
				else
					to_read = remained;

				for (unsigned long i = 0; i < to_read; i++) {
					printf("%c", ((int8_t *)(aux_mini->rw_buffer))[i]);
					remained--;
				}

				aux = aux->next;
			}
			printf("\n");
			return;
		}
		curr = curr->next;
	}
}

// Parseaza inputul de la user in cazul in care primul
// cuvant este WRITE si verifica validitatea comenzii
void parse_write(arena_t *arena, char *user_input, uint64_t *address,
				 uint64_t *size)
{
	char copy_of_input[MAX_INPUT];
	memcpy(copy_of_input, user_input, strlen(user_input) + 1);

	strtok(user_input, "\n "); // sar peste "WRITE"

	char *token = strtok(NULL, "\n ");
	if (!token) {
		treat_error(copy_of_input);
		return;
	}

	// verific daca al doilea parametru este numar si daca nu este negativ
	long dummy = atol(token);
	if ((dummy == 0 && strcmp(token, "0") != 0) || dummy < 0) {
		treat_error(copy_of_input);
		return;
	}

	*address = dummy;

	token = strtok(NULL, "\n ");
	if (!token) {
		treat_error(copy_of_input);
		return;
	}

	// verific al treilea parametru
	dummy = atol(token);
	if ((dummy == 0 && strcmp(token, "0") != 0) || dummy < 0) {
		treat_error(copy_of_input);
		return;
	}

	*size = dummy;

	// daca am ajuns aici, comanda de write este valida
	// bag in data input-ul de la user

	int len = strlen(copy_of_input);
	int8_t *data = malloc((len + 1) * sizeof(*data));
	DIE(!data, "malloc failed\n");

	memcpy(data, copy_of_input, len + 1);

	write(arena, *address, *size, data);
}

// Verifica daca adresa este una valida pentru a scrie in ea
// Returneaza NULL daca nu si blocul in care trebuie sa scriu daca da
block_t *write_validity(arena_t *arena, const uint64_t address)
{
	if (address >= arena->arena_size) {
		printf("Invalid address for write.\n");
		return NULL;
	}

	// parcurg lista de blocuri
	node_t *curr = arena->alloc_list->head;
	int position = 0;
	while (curr) {
		block_t *curr_block = (block_t *)(curr->data);
		// inceput de bloc
		unsigned long block_start = curr_block->start_address;
		// sfarsit de bloc
		unsigned long block_end = block_start + curr_block->size - 1;

		if (address >= block_start && address <= block_end)
			return curr_block;

		curr = curr->next;
		position++;
	}

	// Daca ajung aici, inseamna ca zona de memorie nu este alocata
	printf("Invalid address for write.\n");
	return NULL;
}

// Citeste de la stdin atatea linii de cate e nevoie pentru a atinge
// dimensiunea size si returneaza intregul set de date care trebuie scris
int8_t *get_write_params(const uint64_t size, int8_t *data)
{
	int8_t *input = malloc((size + 1) * sizeof(*input));
	DIE(!input, "malloc failed\n");

	strtok((char *)data, "\n "); // primul param
	strtok(NULL, "\n "); // al doilea param
	strtok(NULL, "\n "); // al treilea param
	char *token = strtok(NULL, "\n"); // iau tot ce mai e pana la endline

	// bag ce a mai fost introdus pe prima linie
	unsigned long index = 0;
	if (token) {
		while (token && *token != '\0') {
			input[index] = *token;
			token++;
			index++;
		}
	}

	if (index < size) {
		input[index] = '\n';
		index++;
	}

	while (index < size) {
		char buffer[MAX_INPUT];
		fgets(buffer, MAX_INPUT, stdin);

		int len = strlen(buffer);
		for (int i = 0; i < len; i++) {
			if (index >= size)
				break;
			input[index] = buffer[i];
			index++;
		}
	}

	input[index] = '\0';

	free(data);

	return input;
}

// Verifica daca un minibloc are permisiuni de scriere
int miniblock_write_perm(miniblock_t *miniblock)
{
	if (miniblock->perm == 2 || miniblock->perm == 3 || miniblock->perm == 6 ||
		miniblock->perm == 7)
		return 1;

	printf("Invalid permissions for write.\n");
	return 0;
}

// Verifica daca vreunul din miniblocurile in care trebuie sa scriu
// nu are permisiuni de scriere
int check_write_perm(node_t *miniblock_node, const uint64_t address,
					 const uint64_t size)
{
	miniblock_t *miniblock = (miniblock_t *)(miniblock_node->data);
	if (miniblock_write_perm(miniblock) == 0)
		return 0;

	// inceput de minibloc
	unsigned long mini_start = miniblock->start_address;
	// sfarsit de minibloc
	unsigned long mini_end = mini_start + miniblock->size - 1;

	// cat mai am de scris
	unsigned long remained = size;
	remained -= mini_end - address + 1; // scad miniblocul actual

	node_t *aux = miniblock_node->next;
	while (aux && remained) {
		miniblock_t *aux_mini = (miniblock_t *)(aux->data);
		if (remained >= aux_mini->size)
			remained -= aux_mini->size;
		else
			remained = 0;
		if (miniblock_write_perm(aux_mini) == 0)
			return 0;
		aux = aux->next;
	}
	return 1;
}

// Scrie <size> din <data> incepand cu <address>
void write(arena_t *arena, const uint64_t address, const uint64_t size,
		   int8_t *data)
{
	data = get_write_params(size, data);

	block_t *to_write_block = write_validity(arena, address);
	if (!to_write_block) {
		free(data);
		return;
	}

	unsigned long space = to_write_block->start_address + to_write_block->size
						- address; // cat pot scrie in acest bloc

	node_t *curr = to_write_block->miniblock_list->head;
	while (curr) {
		miniblock_t *curr_miniblock = (miniblock_t *)(curr->data);
		// inceput de minibloc
		unsigned long mini_start = curr_miniblock->start_address;
		// sfarsit de minibloc
		unsigned long mini_end = mini_start + curr_miniblock->size - 1;

		if (address >= mini_start && address <= mini_end) {
			// verific intai permisiunile pentru fiecare minibloc
			if (check_write_perm(curr, address, size) == 0) {
				free(data);
				return;
			}

			unsigned long remained; // cat mai am de scris
			if (space < size) {
				remained = space;
				printf("Warning: size was bigger than the block size. ");
				printf("Writing %lu characters.\n", space);
			} else {
				remained = size;
			}

			unsigned long cnt = 0; // pozitia in data
			unsigned long to_alloc = 0; // dimensiunea lui rw_buffer

			if (remained >= (mini_end - address + 1))
				to_alloc = mini_end - address + 1;
			else
				to_alloc = remained;

			curr_miniblock->rw_buffer = malloc(to_alloc * sizeof(int8_t));
			DIE(!curr_miniblock->rw_buffer, "malloc failed\n");

			for (unsigned long i = 0; i < to_alloc; i++) {
				((int8_t *)(curr_miniblock->rw_buffer))[i] = data[cnt];
				cnt++;
				remained--;
			}

			node_t *aux = curr->next;
			while (remained && (cnt < size) && aux) {
				miniblock_t *aux_mini = (miniblock_t *)(aux->data);

				to_alloc = 0; // dimensiunea lui rw_buffer
				if (remained >= aux_mini->size)
					to_alloc = aux_mini->size;
				else
					to_alloc = remained;

				aux_mini->rw_buffer = malloc(to_alloc * sizeof(int8_t));
				DIE(!aux_mini->rw_buffer, "malloc failed\n");

				for (unsigned long i = 0; i < to_alloc; i++) {
					((int8_t *)(aux_mini->rw_buffer))[i] = data[cnt];
					cnt++;
					remained--;
				}
				aux = aux->next;
			}
			free(data);
			return;
		}
		curr = curr->next;
	}
}

// Parseaza inputul de la user in cazul in care primul
// cuvant este PMAP si verifica validitatea comenzii
void parse_pmap(char *user_input, int *is_valid)
{
	char copy_of_input[MAX_INPUT];
	memcpy(copy_of_input, user_input, strlen(user_input) + 1);

	*is_valid = 1;
	strtok(user_input, "\n "); // sar peste "PMAP"
	char *token = strtok(NULL, "\n ");

	// verific daca mai exista si alt parametru
	token = strtok(NULL, "\n ");
	if (token) {
		*is_valid = 0;
		treat_error(copy_of_input);
		return;
	}
}

void print_permission(miniblock_t *miniblock)
{
	uint8_t perm = miniblock->perm;
	if (perm == 0)
		printf("---\n");
	if (perm == 1)
		printf("--X\n");
	if (perm == 2)
		printf("-W-\n");
	if (perm == 3)
		printf("-WX\n");
	if (perm == 4)
		printf("R--\n");
	if (perm == 5)
		printf("R-X\n");
	if (perm == 6)
		printf("RW-\n");
	if (perm == 7)
		printf("RWX\n");
}

void pmap(const arena_t *arena)
{
	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	printf("Free memory: 0x%lX bytes\n", arena->free_space);
	printf("Number of allocated blocks: %d\n", arena->block_number);
	printf("Number of allocated miniblocks: %d\n", arena->miniblock_number);
	if (arena->block_number)
		printf("\n");

	int block_position = 1;
	node_t *curr_block = arena->alloc_list->head;
	while (curr_block) {
		printf("Block %d begin\n", block_position);
		block_t *block = (block_t *)(curr_block->data);
		uint64_t block_stop_address = block->start_address + block->size;

		printf("Zone: 0x%lX - 0x%lX\n", block->start_address,
			   block_stop_address);

		int miniblock_position = 1;
		node_t *curr_miniblock = block->miniblock_list->head;
		while (curr_miniblock) {
			printf("Miniblock %d:\t\t", miniblock_position);
			miniblock_t *miniblock = (miniblock_t *)(curr_miniblock->data);

			uint64_t miniblock_stop_address = miniblock->start_address +
											  miniblock->size;

			printf("0x%lX\t\t-\t\t0x%lX\t\t| ", miniblock->start_address,
				   miniblock_stop_address);

			print_permission(miniblock);
			miniblock_position++;
			curr_miniblock = curr_miniblock->next;
		}
		printf("Block %d end\n", block_position);
		if (curr_block->next)
			printf("\n");
		curr_block = curr_block->next;
		block_position++;
	}
}

// Parseaza inputul de la user in cazul in care primul
// cuvant este MPROTECT si verifica validitatea comenzii
void parse_mprotect(arena_t *arena, char *user_input, uint64_t *address)
{
	char copy_of_input[MAX_INPUT];
	memcpy(copy_of_input, user_input, strlen(user_input) + 1);

	strtok(user_input, "\n "); // sar peste "MPROTECT"
	char *token = strtok(NULL, " ");
	if (!token) {
		treat_error(copy_of_input);
		return;
	}

	// verific daca al doilea parametru este numar si daca nu este negativ
	long dummy = atol(token);
	if ((dummy == 0 && strcmp(token, "0") != 0) || dummy < 0) {
		treat_error(copy_of_input);
		return;
	}

	*address = dummy;

	int prot_none = 0;
	int prot_read = 0;
	int prot_write = 0;
	int prot_exec = 0;

	token = strtok(NULL, "|\n ");
	if (!token) {
		treat_error(copy_of_input);
		return;
	}

	while (token) {
		if (strcmp(token, "PROT_NONE") == 0)
			prot_none = 1;
		if (strcmp(token, "PROT_READ") == 0)
			prot_read = 4;
		if (strcmp(token, "PROT_WRITE") == 0)
			prot_write = 2;
		if (strcmp(token, "PROT_EXEC") == 0)
			prot_exec = 1;
		token = strtok(NULL, "|\n ");
	}

	// suma lor imi va da permisiunea
	int sum = prot_read + prot_write + prot_exec;
	int8_t permission = 0;

	if (prot_none) {
		permission = 0;
	} else {
		if (sum == 1)
			permission = 1;
		if (sum == 2)
			permission = 2;
		if (sum == 3)
			permission = 3;
		if (sum == 4)
			permission = 4;
		if (sum == 5)
			permission = 5;
		if (sum == 6)
			permission = 6;
		if (sum == 7)
			permission = 7;
	}
	mprotect(arena, *address, &permission);
}

// Schimba permisiunile miniblocului de la <address> in <permission>
void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	if (address >= arena->arena_size) {
		printf("Invalid address for mprotect.\n");
		return;
	}

	// parcurg lista de blocuri
	node_t *curr = arena->alloc_list->head;
	while (curr) {
		block_t *curr_block = (block_t *)(curr->data);
		// inceput de bloc
		unsigned long block_start = curr_block->start_address;
		// sfarsit de bloc
		unsigned long block_end = block_start + curr_block->size - 1;

		if (address >= block_start && address <= block_end) {
			node_t *mini_curr = curr_block->miniblock_list->head;
			while (mini_curr) {
				miniblock_t *miniblock = (miniblock_t *)(mini_curr->data);
				if (miniblock->start_address == address) {
					miniblock->perm = *permission;
					return;
				}
				mini_curr = mini_curr->next;
			}
			// Daca ajung aici, inseamna ca zona de memorie nu este alocata
			printf("Invalid address for mprotect.\n");
			return;
		}
		curr = curr->next;
	}

	// Daca ajung aici, inseamna ca zona de memorie nu este alocata
	printf("Invalid address for mprotect.\n");
}

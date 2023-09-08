# Marius-Tudor Zaharia 313CAa 2022-2023 
CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g

TARGETS = vma

build: $(TARGETS)

vma: main.o vma.o doubly_list.o
	$(CC) $(CFLAGS) main.o vma.o doubly_list.o -o vma

main.o: main.c
	$(CC) -c $(CFLAGS) main.c

vma.o: vma.c
	$(CC) -c $(CFLAGS) vma.c

doubly_list.o: doubly_list.c
	$(CC) -c $(CFLAGS) doubly_list.c

run_vma: vma
		./vma

pack:
		zip -FSr 313CA_ZahariaMarius-Tudor_Tema1.zip README Makefile *.c *.h

clean:
		rm -f *.o $(TARGETS)

.PHONY: pack clean

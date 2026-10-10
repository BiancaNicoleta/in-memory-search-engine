/**
 * @file main.c
 * @brief Entry point and command loop for the in-memory search engine.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "structs.h"
#include "list_functions.h"
#include "trie.h"
#include "heap.h"
#include "operations.h"

char *read_string_from_file(FILE *fin) {
	int capacity = 10;
	int length = 0;
	int c;

	char *str = (char *)malloc(capacity * sizeof(char));
	if (!str) {
		return NULL;
	}

	while ((c = fgetc(fin)) != EOF && isspace(c)) {
		/* Skip leading whitespace */
	}

	if (c == EOF) {
		free(str);
		return NULL;
	}

	str[length++] = (char)c;
	int in_quotes = (c == '"');

	while (1) {
		c = fgetc(fin);
		if (c == EOF) {
			break;
		}

		if (length + 1 >= capacity) {
			capacity *= 2;
			char *temp = (char *)realloc(str, capacity * sizeof(char));
			if (!temp) {
				free(str);
				return NULL;
			}
			str = temp;
		}

		if (in_quotes) {
			str[length++] = (char)c;
			if (c == '"') {
				break;
			}
		} else {
			if (isspace(c)) {
				break;
			}
			str[length++] = (char)c;
		}
	}

	str[length] = '\0';
	char *ptr = (char *)realloc(str, (length + 1) * sizeof(char));
	if (ptr) {
		str = ptr;
	}

	return str;
}

void execute_command(System *system, char *cmd, FILE *fin, FILE *fout) {
	if (strcmp(cmd, "ADD") == 0) {
		add(system, fin, fout);
	} else if (strcmp(cmd, "DEL") == 0) {
		del(system, fin, fout);
	} else if (strcmp(cmd, "ADDKW") == 0) {
		addkw(system, fin, fout);
	} else if (strcmp(cmd, "DELKW") == 0) {
		delkw(system, fin, fout);
	} else if (strcmp(cmd, "FIND") == 0) {
		find(system, fin, fout);
	} else if (strcmp(cmd, "TOPK") == 0) {
		topk(system, fin, fout);
	} else if (strcmp(cmd, "PRINT") == 0) {
		print(system, fout);
	} else if (strcmp(cmd, "PREFIX") == 0) {
		prefix(system, fin, fout);
	}
	free(cmd);
}

void free_system(System *system) {
	if (!system) {
		return;
	}

	dll_node_t *node = system->files->head;
	while (node != NULL) {
		FileInfo *file = (FileInfo *)node->data;
		free(file->id);

		dll_node_t *kw_node = file->keywords->head;
		while (kw_node != NULL) {
			free(*(char **)kw_node->data);
			kw_node = kw_node->next;
		}
		dll_free(&file->keywords);

		node = node->next;
	}
	dll_free(&system->files);

	free_trie(&system->trie);
	free(system);
}

int main(int argc, char *argv[]) {
	FILE *fin = stdin;
	FILE *fout = stdout;

	if (argc > 1) {
		fin = fopen(argv[1], "r");
		DIE(fin == NULL, "Error opening input file");
	}

	int n_commands;
	if (fscanf(fin, "%d", &n_commands) != 1) {
		if (fin != stdin) {
			fclose(fin);
		}
		return 0;
	}

	System *sys = malloc(sizeof(System));
	DIE(!sys, "malloc System");
	sys->files = dll_create(sizeof(FileInfo));
	sys->trie = create_trie(sizeof(trie_t), 26, "abcdefghijklmnopqrstuvwxyz");

	for (int i = 0; i < n_commands; i++) {
		char *cmd = read_string_from_file(fin);
		if (cmd == NULL) {
			break;
		}
		execute_command(sys, cmd, fin, fout);
	}

	free_system(sys);
	if (fin != stdin) {
		fclose(fin);
	}

	return 0;
}
/**
 * @file main.c
 * @brief Entry point supporting both interactive CLI REPL and batch mode execution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#if defined(_WIN32)
#include <io.h>
#define IS_TERMINAL(stream) _isatty(_fileno(stream))
#else
#include <unistd.h>
#define IS_TERMINAL(stream) isatty(fileno(stream))
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#define strcasecmp _stricmp
#endif

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

static void print_banner(void) {
	printf("==============================================================\n");
	printf("        Trie Search Engine - Interactive Shell v1.0           \n");
	printf("   Type 'help' for command usage, 'exit' or 'quit' to quit.   \n");
	printf("==============================================================\n");
}

static void print_help(void) {
	printf("\nAvailable Commands:\n");
	printf("  add <id> <score> <kw1> [kw2...]   Index a document with score and keywords\n");
	printf("  del <id>                          Remove a document by ID\n");
	printf("  addkw <id> <kw>                   Associate an additional keyword with doc\n");
	printf("  delkw <id> <kw>                   Remove a keyword from a document\n");
	printf("  find <kw> / search <query>        Search keywords (supports 'AND' and 'OR')\n");
	printf("  rank <query>                      Rank query matches by TF-Authority relevance\n");
	printf("  tf <id> <kw>                      Display term frequency of keyword in document\n");
	printf("  topk <kw> <k>                     Retrieve top-K ranked documents by score\n");
	printf("  prefix <prefix>                   Find documents with keywords starting with prefix\n");
	printf("  print / dump                      Display entire keyword dictionary\n");
	printf("  stats                             Display engine statistics\n");
	printf("  help                              Show this help message\n");
	printf("  exit / quit                       Exit the interactive shell\n\n");
}

static reference_list_t *evaluate_query_tokens(System *sys, char **tokens, int num_tokens, char *op_name) {
	int is_or = 0;
	for (int i = 1; i < num_tokens; i++) {
		if (strcasecmp(tokens[i], "OR") == 0) {
			is_or = 1;
			break;
		}
	}

	char *terms[64];
	int term_count = 0;
	for (int i = 1; i < num_tokens && term_count < 64; i++) {
		if (strcasecmp(tokens[i], "AND") == 0 || strcasecmp(tokens[i], "OR") == 0) {
			continue;
		}
		terms[term_count++] = tokens[i];
	}

	if (term_count == 0) {
		return NULL;
	}

	if (op_name) {
		strcpy(op_name, is_or ? "OR" : "AND");
	}

	if (is_or) {
		return search_boolean_or(sys, terms, term_count);
	} else {
		return search_boolean_and(sys, terms, term_count);
	}
}

void run_interactive_shell(System *sys) {
	char line[1024];
	print_banner();

	while (1) {
		printf("search> ");
		fflush(stdout);

		if (!fgets(line, sizeof(line), stdin)) {
			printf("\nExiting search engine. Goodbye!\n");
			break;
		}

		/* Strip trailing newlines and carriage returns */
		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
			line[--len] = '\0';
		}

		/* Tokenize line by whitespace (supporting quoted tokens) */
		char *tokens[128];
		int num_tokens = 0;
		char *p = line;

		while (*p && isspace((unsigned char)*p)) {
			p++;
		}
		if (*p == '\0') {
			continue;
		}

		while (*p && num_tokens < 128) {
			while (*p && isspace((unsigned char)*p)) {
				p++;
			}
			if (*p == '\0') {
				break;
			}

			if (*p == '"') {
				p++;
				tokens[num_tokens++] = p;
				while (*p && *p != '"') {
					p++;
				}
				if (*p == '"') {
					*p = '\0';
					p++;
				}
			} else {
				tokens[num_tokens++] = p;
				while (*p && !isspace((unsigned char)*p)) {
					p++;
				}
				if (*p) {
					*p = '\0';
					p++;
				}
			}
		}

		if (num_tokens == 0) {
			continue;
		}

		char *cmd = tokens[0];

		if (strcasecmp(cmd, "exit") == 0 || strcasecmp(cmd, "quit") == 0) {
			printf("Exiting search engine. Goodbye!\n");
			break;
		} else if (strcasecmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
			print_help();
		} else if (strcasecmp(cmd, "stats") == 0 || strcasecmp(cmd, "status") == 0) {
			printf("--- Search Engine Statistics ---\n");
			printf("  Indexed Documents : %u\n", sys->files ? sys->files->size : 0);
			printf("  Unique Keywords   : %d\n", sys->trie ? sys->trie->size : 0);
			printf("  Trie Nodes Count  : %d\n", sys->trie ? sys->trie->nNodes : 0);
			printf("--------------------------------\n");
		} else if (strcasecmp(cmd, "add") == 0) {
			if (num_tokens < 4) {
				printf("[USAGE] add <id> <score> <kw1> [kw2 ...]\n");
				continue;
			}
			char *id = tokens[1];
			int score = atoi(tokens[2]);

			int start_kw = 3;
			int count = num_tokens - 3;
			char *endptr;
			long explicit_count = strtol(tokens[3], &endptr, 10);
			if (*endptr == '\0' && explicit_count == num_tokens - 4) {
				start_kw = 4;
				count = num_tokens - 4;
			}

			if (count <= 0) {
				printf("[ERROR] At least one keyword is required.\n");
				continue;
			}

			if (add_document(sys, id, score, count, &tokens[start_kw])) {
				printf("[OK] Document '%s' added (Score: %d, %d keyword(s)).\n", id, score, count);
			} else {
				printf("[EXISTS] Document '%s' already exists in index.\n", id);
			}
		} else if (strcasecmp(cmd, "del") == 0) {
			if (num_tokens < 2) {
				printf("[USAGE] del <id>\n");
				continue;
			}
			if (delete_document(sys, tokens[1])) {
				printf("[OK] Document '%s' removed.\n", tokens[1]);
			} else {
				printf("[NOT FOUND] Document '%s' does not exist.\n", tokens[1]);
			}
		} else if (strcasecmp(cmd, "addkw") == 0) {
			if (num_tokens < 3) {
				printf("[USAGE] addkw <id> <keyword>\n");
				continue;
			}
			if (add_keyword_to_doc(sys, tokens[1], tokens[2])) {
				printf("[OK] Keyword '%s' added to '%s'.\n", tokens[2], tokens[1]);
			} else {
				printf("[NOT FOUND] Document '%s' does not exist.\n", tokens[1]);
			}
		} else if (strcasecmp(cmd, "delkw") == 0) {
			if (num_tokens < 3) {
				printf("[USAGE] delkw <id> <keyword>\n");
				continue;
			}
			if (delete_keyword_from_doc(sys, tokens[1], tokens[2])) {
				printf("[OK] Keyword '%s' removed from '%s'.\n", tokens[2], tokens[1]);
			} else {
				printf("[NOT FOUND] Document '%s' does not exist.\n", tokens[1]);
			}
		} else if (strcasecmp(cmd, "tf") == 0) {
			if (num_tokens < 3) {
				printf("[USAGE] tf <id> <keyword>\n");
				continue;
			}
			int freq = get_term_frequency(sys, tokens[1], tokens[2]);
			printf("Document '%s' -> Keyword '%s': Term Frequency (TF) = %d\n",
			       tokens[1], tokens[2], freq);
		} else if (strcasecmp(cmd, "find") == 0 || strcasecmp(cmd, "search") == 0) {
			if (num_tokens < 2) {
				printf("[USAGE] search <kw1> [AND|OR kw2 ...]\n");
				continue;
			}
			char op_name[8] = "AND";
			reference_list_t *list = evaluate_query_tokens(sys, tokens, num_tokens, op_name);
			if (!list || list->size == 0) {
				printf("No matching documents found.\n");
			} else {
				printf("Found %u document(s) (Boolean Mode: %s):\n", list->size, op_name);
				reference_node_t *curr = list->head;
				while (curr) {
					printf("  - %-15s (Base Score: %3d, TF: %2d)\n",
					       curr->file->id, curr->file->score, curr->tf);
					curr = curr->next;
				}
			}
			if (list) {
				ref_list_free(&list);
			}
		} else if (strcasecmp(cmd, "rank") == 0) {
			if (num_tokens < 2) {
				printf("[USAGE] rank <kw1> [AND|OR kw2 ...]\n");
				continue;
			}
			char op_name[8] = "AND";
			reference_list_t *list = evaluate_query_tokens(sys, tokens, num_tokens, op_name);
			if (!list || list->size == 0) {
				printf("No matching documents found.\n");
			} else {
				int count = 0;
				SearchResult *results = rank_results_by_tf(list, &count);
				printf("Ranked Results (Mode: %s | Relevance = Base + TF * 10):\n", op_name);
				for (int i = 0; i < count; i++) {
					printf("  %d. %-15s [Relevance: %3d | Base Score: %3d | Total TF: %2d]\n",
					       i + 1, results[i].file->id, results[i].relevance_score,
					       results[i].base_score, results[i].total_tf);
				}
				free(results);
			}
			if (list) {
				ref_list_free(&list);
			}
		} else if (strcasecmp(cmd, "topk") == 0) {
			if (num_tokens < 3) {
				printf("[USAGE] topk <keyword> <k>\n");
				continue;
			}
			char *kw = tokens[1];
			int k = atoi(tokens[2]);
			if (k <= 0 && atoi(tokens[1]) > 0) {
				k = atoi(tokens[1]);
				kw = tokens[2];
			}

			trie_node_t *node = search_key(sys->trie, kw);
			if (!node || !node->ref_list || node->ref_list->size == 0) {
				printf("No documents found for keyword '%s'.\n", kw);
			} else {
				reference_list_t *list = node->ref_list;
				reference_node_t *curr = list->head;
				heap_t *heap = create_heap(cmp_files, sizeof(FileInfo *));
				for (unsigned int i = 0; i < list->size; i++) {
					insert_heap(heap, &curr->file);
					curr = curr->next;
				}
				int limit = k > (int)list->size ? (int)list->size : k;
				printf("Top %d ranked document(s) for '%s':\n", limit, kw);
				for (int i = 0; i < limit; i++) {
					FileInfo *top_file = *(FileInfo **)top(heap);
					printf("  %d. [Score: %3d] %s\n", i + 1, top_file->score, top_file->id);
					pop(heap);
				}
				free_heap(heap);
			}
		} else if (strcasecmp(cmd, "prefix") == 0) {
			if (num_tokens < 2) {
				printf("[USAGE] prefix <prefix>\n");
				continue;
			}
			char *pref = tokens[1];
			trie_node_t *node = search_prefix(sys->trie, pref);
			if (!node) {
				printf("No keywords found matching prefix '%s'.\n", pref);
			} else {
				reference_list_t *ref_list = ref_list_create();
				collect_prefix_references(sys, node, ref_list);
				if (ref_list->size == 0) {
					printf("No documents found matching prefix '%s'.\n", pref);
				} else {
					char **id_list = malloc(ref_list->size * sizeof(char *));
					DIE(!id_list, "malloc id_list");
					reference_node_t *r_node = ref_list->head;
					for (unsigned int i = 0; i < ref_list->size; i++) {
						id_list[i] = r_node->file->id;
						r_node = r_node->next;
					}
					qsort(id_list, ref_list->size, sizeof(char *), cmp_str);
					printf("Found %u document(s) matching prefix '%s':\n", ref_list->size, pref);
					for (unsigned int i = 0; i < ref_list->size; i++) {
						printf("  - %s\n", id_list[i]);
					}
					free(id_list);
				}
				ref_list_free(&ref_list);
			}
		} else if (strcasecmp(cmd, "print") == 0 || strcasecmp(cmd, "dump") == 0) {
			printf("--- Inverted Index Dictionary ---\n");
			print(sys, stdout);
			printf("----------------------------------\n");
		} else {
			printf("[UNKNOWN COMMAND] '%s'. Type 'help' for available commands.\n", cmd);
		}
	}
}

static void run_batch_mode(System *sys, FILE *fin, FILE *fout) {
	int n_commands;
	if (fscanf(fin, "%d", &n_commands) != 1) {
		return;
	}

	for (int i = 0; i < n_commands; i++) {
		char *cmd = read_string_from_file(fin);
		if (cmd == NULL) {
			break;
		}
		execute_command(sys, cmd, fin, fout);
	}
}

int main(int argc, char *argv[]) {
	System *sys = malloc(sizeof(System));
	DIE(!sys, "malloc System");
	sys->files = dll_create(sizeof(FileInfo));
	sys->trie = create_trie(sizeof(trie_t), 26, "abcdefghijklmnopqrstuvwxyz");

	if (argc > 1) {
		if (strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "--interactive") == 0) {
			run_interactive_shell(sys);
		} else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			printf("Usage: search_index [OPTIONS] [INPUT_FILE]\n\n");
			printf("Options:\n");
			printf("  -i, --interactive   Launch interactive REPL prompt\n");
			printf("  -h, --help          Show this help message\n\n");
			printf("If INPUT_FILE is provided, commands are executed in batch mode.\n");
			printf("If run without arguments in a terminal, launches interactive REPL.\n");
		} else {
			FILE *fin = fopen(argv[1], "r");
			DIE(fin == NULL, "Error opening input file");
			run_batch_mode(sys, fin, stdout);
			fclose(fin);
		}
	} else {
		FILE *f_check = fopen("indexare.in", "r");
		if (f_check) {
			FILE *fout = fopen("indexare.out", "w");
			DIE(fout == NULL, "Error opening indexare.out");
			run_batch_mode(sys, f_check, fout);
			fclose(f_check);
			fclose(fout);
		} else if (IS_TERMINAL(stdin)) {
			run_interactive_shell(sys);
		} else {
			run_batch_mode(sys, stdin, stdout);
		}
	}

	free_system(sys);
	return 0;
}
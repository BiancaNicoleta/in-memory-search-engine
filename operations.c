/**
 * @file operations.c
 * @brief Implementations of search engine command handlers and indexing logic.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "list_functions.h"
#include "trie.h"
#include "heap.h"
#include "operations.h"

/**
 * @brief String comparison helper for qsort on arrays of string pointers.
 */
static int cmp_str(const void *a, const void *b) {
	return strcmp(*(const char **)a, *(const char **)b);
}

FileInfo *get_file_by_id(System *system, const char *id, int *out_index) {
	for (unsigned int i = 0; i < system->files->size; i++) {
		dll_node_t *node = dll_get_nth_node(system->files, i);
		FileInfo *file = (FileInfo *)node->data;
		if (strcmp(file->id, id) == 0) {
			if (out_index) {
				*out_index = (int)i;
			}
			return file;
		}
	}
	return NULL;
}

int get_keyword_index(FileInfo *file, const char *keyword) {
	for (unsigned int j = 0; j < file->keywords->size; j++) {
		dll_node_t *kw_node = dll_get_nth_node(file->keywords, j);
		char *kw = *(char **)kw_node->data;
		if (strcmp(kw, keyword) == 0) {
			return (int)j;
		}
	}
	return -1;
}

void remove_file_from_ref_list(reference_list_t *list, const char *id) {
	if (!list || list->size == 0) {
		return;
	}

	reference_node_t *curr = list->head;
	reference_node_t *prev = NULL;

	while (curr) {
		if (strcmp(curr->file->id, id) == 0) {
			if (prev) {
				prev->next = curr->next;
			} else {
				list->head = curr->next;
			}
			free(curr);
			list->size--;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}

void add(System *sys, FILE *fin, FILE *fout) {
	int score, t;
	char *id = read_string_from_file(fin);
	if (!id) {
		return;
	}

	if (fscanf(fin, "%d %d", &score, &t) != 2) {
		free(id);
		return;
	}

	if (get_file_by_id(sys, id, NULL) != NULL) {
		fprintf(fout, "EXISTS\n");
		for (int i = 0; i < t; i++) {
			char *kw = read_string_from_file(fin);
			free(kw);
		}
		free(id);
		return;
	}

	FileInfo *new_file = (FileInfo *)malloc(sizeof(FileInfo));
	DIE(!new_file, "malloc FileInfo");
	new_file->id = id;
	new_file->score = score;
	new_file->keywords = dll_create(sizeof(char *));
	dll_add_nth_node(sys->files, sys->files->size, new_file);
	free(new_file);

	dll_node_t *node = dll_get_nth_node(sys->files, sys->files->size - 1);
	FileInfo *file = (FileInfo *)node->data;
	for (int i = 0; i < t; i++) {
		char *keyword = read_string_from_file(fin);
		if (!keyword) {
			continue;
		}

		if (get_keyword_index(file, keyword) == -1) {
			insert_key(sys->trie, keyword, file);

			char *kw_copy = (char *)malloc(strlen(keyword) + 1);
			DIE(!kw_copy, "malloc kw_copy");
			strcpy(kw_copy, keyword);
			dll_add_nth_node(file->keywords, file->keywords->size, &kw_copy);
		}
		free(keyword);
	}
	fprintf(fout, "OK\n");
}

void del(System *system, FILE *fin, FILE *fout) {
	char *id = read_string_from_file(fin);
	if (!id) {
		return;
	}

	int file_idx = -1;
	FileInfo *file = get_file_by_id(system, id, &file_idx);

	if (!file) {
		fprintf(fout, "NOT FOUND\n");
		free(id);
		return;
	}

	for (int i = (int)file->keywords->size - 1; i >= 0; i--) {
		dll_node_t *kw_node = dll_get_nth_node(file->keywords, (unsigned int)i);
		char *keyword = *(char **)kw_node->data;

		trie_node_t *trie_node = search_key(system->trie, keyword);
		if (trie_node && trie_node->ref_list) {
			remove_file_from_ref_list(trie_node->ref_list, id);
			if (!trie_node->ref_list->size) {
				ref_list_free(&trie_node->ref_list);
				delete_key(system->trie, system->trie->root, keyword);
			}
		}
		free(keyword);
	}
	dll_free(&file->keywords);

	free(file->id);
	dll_node_t *removed = dll_remove_nth_node(system->files, (unsigned int)file_idx);
	dll_free_node(&removed);
	free(id);

	fprintf(fout, "OK\n");
}

void addkw(System *system, FILE *fin, FILE *fout) {
	char *id = read_string_from_file(fin);
	char *keyword_to_add = read_string_from_file(fin);
	if (!id || !keyword_to_add) {
		free(id);
		free(keyword_to_add);
		return;
	}

	FileInfo *file = get_file_by_id(system, id, NULL);
	if (!file) {
		fprintf(fout, "NOT FOUND\n");
		free(id);
		free(keyword_to_add);
		return;
	}

	int kw_idx = get_keyword_index(file, keyword_to_add);
	if (kw_idx == -1) {
		insert_key(system->trie, keyword_to_add, file);

		char *kw_copy = (char *)malloc(strlen(keyword_to_add) + 1);
		DIE(!kw_copy, "malloc kw_copy");
		strcpy(kw_copy, keyword_to_add);
		dll_add_nth_node(file->keywords, file->keywords->size, &kw_copy);
	}
	fprintf(fout, "OK\n");

	free(keyword_to_add);
	free(id);
}

void delkw(System *system, FILE *fin, FILE *fout) {
	char *id = read_string_from_file(fin);
	char *keyword_to_delete = read_string_from_file(fin);
	if (!id || !keyword_to_delete) {
		free(id);
		free(keyword_to_delete);
		return;
	}

	int file_idx = -1;
	FileInfo *file = get_file_by_id(system, id, &file_idx);
	if (!file) {
		fprintf(fout, "NOT FOUND\n");
		free(id);
		free(keyword_to_delete);
		return;
	}

	int kw_idx = get_keyword_index(file, keyword_to_delete);
	if (kw_idx == -1) {
		fprintf(fout, "OK\n");
		free(id);
		free(keyword_to_delete);
		return;
	}

	dll_node_t *removed_kw = dll_remove_nth_node(file->keywords, (unsigned int)kw_idx);
	free(*(char **)removed_kw->data);
	dll_free_node(&removed_kw);

	trie_node_t *trie_node = search_key(system->trie, keyword_to_delete);
	if (trie_node && trie_node->ref_list) {
		remove_file_from_ref_list(trie_node->ref_list, id);
		if (!trie_node->ref_list->size) {
			ref_list_free(&trie_node->ref_list);
			delete_key(system->trie, system->trie->root, keyword_to_delete);
		}
	}

	if (!file->keywords->size) {
		dll_free(&file->keywords);
		free(file->id);
		dll_node_t *removed = dll_remove_nth_node(system->files, (unsigned int)file_idx);
		dll_free_node(&removed);
	}

	fprintf(fout, "OK\n");
	free(keyword_to_delete);
	free(id);
}

void find(System *system, FILE *fin, FILE *fout) {
	char *keyword_to_find = read_string_from_file(fin);
	if (!keyword_to_find) {
		return;
	}

	trie_node_t *t_node = search_key(system->trie, keyword_to_find);
	if (!t_node || !t_node->ref_list || t_node->ref_list->size == 0) {
		fprintf(fout, "EMPTY\n");
		free(keyword_to_find);
		return;
	}

	reference_list_t *list = t_node->ref_list;
	reference_node_t *curr = list->head;

	char **id_list = (char **)malloc(list->size * sizeof(char *));
	DIE(!id_list, "malloc id_list");
	for (unsigned int i = 0; i < list->size; i++) {
		id_list[i] = curr->file->id;
		curr = curr->next;
	}

	qsort(id_list, list->size, sizeof(char *), cmp_str);

	fprintf(fout, "%u ", list->size);
	for (unsigned int i = 0; i < list->size; i++) {
		fprintf(fout, "%s ", id_list[i]);
	}
	fprintf(fout, "\n");

	free(id_list);
	free(keyword_to_find);
}

int cmp_files(const void *a, const void *b) {
	FileInfo *file_a = *(FileInfo **)a;
	FileInfo *file_b = *(FileInfo **)b;
	if (file_a->score != file_b->score) {
		return file_a->score - file_b->score;
	}
	return strcmp(file_b->id, file_a->id);
}

void topk(System *system, FILE *fin, FILE *fout) {
	char *keyword = read_string_from_file(fin);
	if (!keyword) {
		return;
	}

	int k;
	if (fscanf(fin, "%d", &k) != 1) {
		free(keyword);
		return;
	}

	trie_node_t *t_node = search_key(system->trie, keyword);
	if (!t_node || !t_node->ref_list || t_node->ref_list->size == 0) {
		fprintf(fout, "EMPTY\n");
		free(keyword);
		return;
	}

	reference_list_t *list = t_node->ref_list;
	reference_node_t *curr = list->head;

	heap_t *heap = create_heap(cmp_files, sizeof(FileInfo *));
	for (unsigned int i = 0; i < list->size; i++) {
		insert_heap(heap, &curr->file);
		curr = curr->next;
	}

	k = k > (int)list->size ? (int)list->size : k;

	fprintf(fout, "%d ", k);
	for (int i = 0; i < k; i++) {
		FileInfo *top_file = *(FileInfo **)top(heap);
		fprintf(fout, "%s ", top_file->id);
		pop(heap);
	}
	fprintf(fout, "\n");

	free(keyword);
	free_heap(heap);
}

void print_h(trie_t *trie, trie_node_t *curr, FILE *fout, char *key, size_t key_size) {
	for (int i = 0; i < trie->alphabet_size; i++) {
		if (curr->children[i]) {
			key[key_size] = trie->alphabet[i];
			key_size++;
			if (curr->children[i]->end_of_word) {
				key[key_size] = '\0';
				fprintf(fout, "%s %u ", key, curr->children[i]->ref_list->size);
				reference_node_t *ref_curr = curr->children[i]->ref_list->head;
				unsigned int num_files = curr->children[i]->ref_list->size;

				char **id_list = (char **)malloc(num_files * sizeof(char *));
				DIE(!id_list, "malloc id_list");
				for (unsigned int j = 0; j < num_files; j++) {
					id_list[j] = ref_curr->file->id;
					ref_curr = ref_curr->next;
				}

				qsort(id_list, num_files, sizeof(char *), cmp_str);

				for (unsigned int j = 0; j < num_files; j++) {
					fprintf(fout, "%s ", id_list[j]);
				}
				free(id_list);
				fprintf(fout, "\n");
			}
			print_h(trie, curr->children[i], fout, key, key_size);
			key_size--;
		}
	}
}

void print(System *system, FILE *fout) {
	trie_t *trie = system->trie;
	if (trie->size == 0) {
		fprintf(fout, "EMPTY\n");
		return;
	}
	char key[101] = {0};
	print_h(trie, trie->root, fout, key, 0);
}

void collect_prefix_references(System *system, trie_node_t *curr, reference_list_t *list) {
	if (curr->end_of_word && curr->ref_list) {
		reference_node_t *ref_curr = curr->ref_list->head;
		unsigned int num_files = curr->ref_list->size;

		for (unsigned int j = 0; j < num_files; j++) {
			if (!ref_list_check_node_exists(list, ref_curr->file)) {
				ref_list_add_nth_node(list, list->size, ref_curr->file);
			}
			ref_curr = ref_curr->next;
		}
	}

	for (int i = 0; i < system->trie->alphabet_size; i++) {
		if (curr->children[i]) {
			collect_prefix_references(system, curr->children[i], list);
		}
	}
}

void prefix(System *system, FILE *fin, FILE *fout) {
	char *prefix_str = read_string_from_file(fin);
	if (!prefix_str) {
		return;
	}

	trie_node_t *t_node = search_prefix(system->trie, prefix_str);
	if (!t_node) {
		fprintf(fout, "EMPTY\n");
		free(prefix_str);
		return;
	}

	reference_list_t *ref_list = ref_list_create();
	collect_prefix_references(system, t_node, ref_list);

	if (ref_list->size == 0) {
		fprintf(fout, "EMPTY\n");
	} else {
		char **id_list = (char **)malloc(ref_list->size * sizeof(char *));
		DIE(!id_list, "malloc id_list");
		reference_node_t *node = ref_list->head;
		for (unsigned int i = 0; i < ref_list->size; i++) {
			id_list[i] = node->file->id;
			node = node->next;
		}

		qsort(id_list, ref_list->size, sizeof(char *), cmp_str);

		fprintf(fout, "%u ", ref_list->size);
		for (unsigned int i = 0; i < ref_list->size; i++) {
			fprintf(fout, "%s ", id_list[i]);
		}
		fprintf(fout, "\n");
		free(id_list);
	}

	free(prefix_str);
	ref_list_free(&ref_list);
}

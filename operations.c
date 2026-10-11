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
int cmp_str(const void *a, const void *b) {
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

int add_document(System *sys, const char *id, int score, int kw_count, char **keywords) {
	if (get_file_by_id(sys, id, NULL) != NULL) {
		return 0;
	}

	FileInfo *new_file = (FileInfo *)malloc(sizeof(FileInfo));
	DIE(!new_file, "malloc FileInfo");
	new_file->id = (char *)malloc(strlen(id) + 1);
	DIE(!new_file->id, "malloc id");
	strcpy(new_file->id, id);
	new_file->score = score;
	new_file->keywords = dll_create(sizeof(char *));
	dll_add_nth_node(sys->files, sys->files->size, new_file);
	free(new_file);

	dll_node_t *node = dll_get_nth_node(sys->files, sys->files->size - 1);
	FileInfo *file = (FileInfo *)node->data;
	for (int i = 0; i < kw_count; i++) {
		if (!keywords[i]) {
			continue;
		}
		if (get_keyword_index(file, keywords[i]) == -1) {
			insert_key(sys->trie, keywords[i], file);

			char *kw_copy = (char *)malloc(strlen(keywords[i]) + 1);
			DIE(!kw_copy, "malloc kw_copy");
			strcpy(kw_copy, keywords[i]);
			dll_add_nth_node(file->keywords, file->keywords->size, &kw_copy);
		}
	}
	return 1;
}

int delete_document(System *system, const char *id) {
	int file_idx = -1;
	FileInfo *file = get_file_by_id(system, id, &file_idx);
	if (!file) {
		return 0;
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
	return 1;
}

int add_keyword_to_doc(System *system, const char *id, const char *keyword) {
	FileInfo *file = get_file_by_id(system, id, NULL);
	if (!file) {
		return 0;
	}

	int kw_idx = get_keyword_index(file, keyword);
	if (kw_idx == -1) {
		insert_key(system->trie, (char *)keyword, file);

		char *kw_copy = (char *)malloc(strlen(keyword) + 1);
		DIE(!kw_copy, "malloc kw_copy");
		strcpy(kw_copy, keyword);
		dll_add_nth_node(file->keywords, file->keywords->size, &kw_copy);
	}
	return 1;
}

int delete_keyword_from_doc(System *system, const char *id, const char *keyword) {
	int file_idx = -1;
	FileInfo *file = get_file_by_id(system, id, &file_idx);
	if (!file) {
		return 0;
	}

	int kw_idx = get_keyword_index(file, keyword);
	if (kw_idx == -1) {
		return 1;
	}

	dll_node_t *removed_kw = dll_remove_nth_node(file->keywords, (unsigned int)kw_idx);
	free(*(char **)removed_kw->data);
	dll_free_node(&removed_kw);

	trie_node_t *trie_node = search_key(system->trie, (char *)keyword);
	if (trie_node && trie_node->ref_list) {
		remove_file_from_ref_list(trie_node->ref_list, id);
		if (!trie_node->ref_list->size) {
			ref_list_free(&trie_node->ref_list);
			delete_key(system->trie, system->trie->root, (char *)keyword);
		}
	}

	if (!file->keywords->size) {
		dll_free(&file->keywords);
		free(file->id);
		dll_node_t *removed = dll_remove_nth_node(system->files, (unsigned int)file_idx);
		dll_free_node(&removed);
	}
	return 1;
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

	if (delete_document(system, id)) {
		fprintf(fout, "OK\n");
	} else {
		fprintf(fout, "NOT FOUND\n");
	}
	free(id);
}

void addkw(System *system, FILE *fin, FILE *fout) {
	char *id = read_string_from_file(fin);
	char *keyword_to_add = read_string_from_file(fin);
	if (!id || !keyword_to_add) {
		free(id);
		free(keyword_to_add);
		return;
	}

	if (add_keyword_to_doc(system, id, keyword_to_add)) {
		fprintf(fout, "OK\n");
	} else {
		fprintf(fout, "NOT FOUND\n");
	}

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

	if (get_file_by_id(system, id, NULL) == NULL) {
		fprintf(fout, "NOT FOUND\n");
	} else {
		delete_keyword_from_doc(system, id, keyword_to_delete);
		fprintf(fout, "OK\n");
	}

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

int get_term_frequency(System *system, const char *doc_id, const char *keyword) {
	if (!system || !doc_id || !keyword) {
		return 0;
	}

	trie_node_t *node = search_key(system->trie, (char *)keyword);
	if (!node || !node->ref_list) {
		return 0;
	}

	reference_node_t *curr = node->ref_list->head;
	while (curr) {
		if (strcmp(curr->file->id, doc_id) == 0) {
			return curr->tf;
		}
		curr = curr->next;
	}
	return 0;
}

reference_list_t *search_boolean_and(System *system, char **keywords, int kw_count) {
	if (!system || !keywords || kw_count <= 0) {
		return ref_list_create();
	}

	trie_node_t **nodes = malloc(kw_count * sizeof(trie_node_t *));
	DIE(!nodes, "malloc nodes");

	for (int i = 0; i < kw_count; i++) {
		nodes[i] = search_key(system->trie, keywords[i]);
		if (!nodes[i] || !nodes[i]->ref_list || nodes[i]->ref_list->size == 0) {
			free(nodes);
			return ref_list_create();
		}
	}

	reference_list_t *res = ref_list_create();
	reference_node_t *candidate = nodes[0]->ref_list->head;

	while (candidate) {
		int matches_all = 1;
		int total_tf = candidate->tf;

		for (int i = 1; i < kw_count; i++) {
			reference_node_t *matched = ref_list_check_node_exists(nodes[i]->ref_list, candidate->file);
			if (!matched) {
				matches_all = 0;
				break;
			}
			total_tf += matched->tf;
		}

		if (matches_all) {
			ref_list_add_nth_node(res, res->size, candidate->file);
			reference_node_t *added = ref_list_check_node_exists(res, candidate->file);
			if (added) {
				added->tf = total_tf;
			}
		}

		candidate = candidate->next;
	}

	free(nodes);
	return res;
}

reference_list_t *search_boolean_or(System *system, char **keywords, int kw_count) {
	reference_list_t *res = ref_list_create();
	if (!system || !keywords || kw_count <= 0) {
		return res;
	}

	for (int i = 0; i < kw_count; i++) {
		trie_node_t *node = search_key(system->trie, keywords[i]);
		if (!node || !node->ref_list) {
			continue;
		}

		reference_node_t *curr = node->ref_list->head;
		while (curr) {
			reference_node_t *existing = ref_list_check_node_exists(res, curr->file);
			if (existing) {
				existing->tf += curr->tf;
			} else {
				ref_list_add_nth_node(res, res->size, curr->file);
				reference_node_t *added = ref_list_check_node_exists(res, curr->file);
				if (added) {
					added->tf = curr->tf;
				}
			}
			curr = curr->next;
		}
	}

	return res;
}

static int cmp_search_results(const void *a, const void *b) {
	const SearchResult *ra = (const SearchResult *)a;
	const SearchResult *rb = (const SearchResult *)b;
	if (ra->relevance_score != rb->relevance_score) {
		return rb->relevance_score - ra->relevance_score;
	}
	return strcmp(ra->file->id, rb->file->id);
}

SearchResult *rank_results_by_tf(reference_list_t *list, int *out_count) {
	if (!list || list->size == 0) {
		if (out_count) {
			*out_count = 0;
		}
		return NULL;
	}

	int n = (int)list->size;
	SearchResult *results = malloc(n * sizeof(SearchResult));
	DIE(!results, "malloc SearchResult");

	reference_node_t *curr = list->head;
	for (int i = 0; i < n; i++) {
		results[i].file = curr->file;
		results[i].base_score = curr->file->score;
		results[i].total_tf = curr->tf;
		/* Composite relevance score: Base score + (TF * 10) */
		results[i].relevance_score = curr->file->score + (curr->tf * 10);
		curr = curr->next;
	}

	qsort(results, n, sizeof(SearchResult), cmp_search_results);

	if (out_count) {
		*out_count = n;
	}
	return results;
}

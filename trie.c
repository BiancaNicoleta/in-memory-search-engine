/**
 * @file trie.c
 * @brief Prefix tree (Trie) implementation for indexing and keyword searches.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list_functions.h"
#include "structs.h"
#include "trie.h"

trie_node_t *trie_create_node(trie_t *trie) {
	trie_node_t *node = (trie_node_t *)malloc(sizeof(trie_node_t));
	DIE(!node, "trie_create_node malloc");

	node->ref_list = NULL;
	node->end_of_word = 0;
	node->children = (trie_node_t **)malloc(sizeof(trie_node_t *) * trie->alphabet_size);
	DIE(!node->children, "trie_create_node children malloc");

	for (int i = 0; i < trie->alphabet_size; i++) {
		node->children[i] = NULL;
	}
	node->n_children = 0;
	return node;
}

trie_t *create_trie(int data_size, int alphabet_size, char *alphabet) {
	trie_t *trie = (trie_t *)malloc(sizeof(trie_t));
	DIE(!trie, "create_trie malloc");

	trie->data_size = data_size;
	trie->alphabet_size = alphabet_size;
	trie->alphabet = alphabet;
	trie->root = trie_create_node(trie);
	trie->size = 0;
	trie->nNodes = 1;
	return trie;
}

void insert_key(trie_t *trie, char *key, void *value) {
	trie_node_t *curr = trie->root;
	size_t len = strlen(key);

	for (size_t i = 0; i < len; i++) {
		char c = key[i];
		int index = c - 'a';
		if (index < 0 || index >= trie->alphabet_size) {
			continue;
		}

		if (curr->children[index] == NULL) {
			curr->children[index] = trie_create_node(trie);
			curr->n_children++;
			trie->nNodes++;
		}

		curr = curr->children[index];
	}

	if (curr->ref_list == NULL) {
		curr->ref_list = ref_list_create();
	}
	reference_node_t *existing = ref_list_check_node_exists(curr->ref_list, (FileInfo *)value);
	if (existing) {
		existing->tf++;
	} else {
		ref_list_add_nth_node(curr->ref_list, curr->ref_list->size, value);
	}

	if (curr->end_of_word == 0) {
		curr->end_of_word = 1;
		trie->size++;
	}
}

trie_node_t *search_key(trie_t *trie, char *key) {
	trie_node_t *curr = trie->root;
	size_t len = strlen(key);

	for (size_t i = 0; i < len; i++) {
		char c = key[i];
		int index = c - 'a';
		if (index < 0 || index >= trie->alphabet_size) {
			return NULL;
		}

		if (curr->children[index] == NULL) {
			return NULL;
		}

		curr = curr->children[index];
	}

	if (curr->end_of_word == 1) {
		return curr;
	}

	return NULL;
}

trie_node_t *search_prefix(trie_t *trie, char *prefix) {
	trie_node_t *curr = trie->root;
	size_t len = strlen(prefix);

	for (size_t i = 0; i < len; i++) {
		char c = prefix[i];
		int index = c - 'a';
		if (index < 0 || index >= trie->alphabet_size) {
			return NULL;
		}

		if (curr->children[index] == NULL) {
			return NULL;
		}

		curr = curr->children[index];
	}

	return curr;
}

int delete_key(trie_t *trie, trie_node_t *curr, char *key) {
	if (strlen(key) == 0) {
		if (curr->ref_list) {
			ref_list_free(&curr->ref_list);
		}

		if (curr->end_of_word == 1) {
			curr->ref_list = NULL;
			curr->end_of_word = 0;
			trie->size--;

			return curr->n_children == 0;
		}

		return 0;
	}

	int index = key[0] - 'a';
	if (index < 0 || index >= trie->alphabet_size) {
		return 0;
	}

	trie_node_t *nextNode = curr->children[index];

	if (nextNode != NULL && delete_key(trie, nextNode, key + 1) == 1) {
		free(nextNode->children);
		free(nextNode);

		curr->children[index] = NULL;
		curr->n_children--;
		trie->nNodes--;

		if (curr->n_children == 0 && curr->end_of_word == 0) {
			return 1;
		}
	}

	return 0;
}

void free_trie_h(trie_t *trie, trie_node_t *curr) {
	for (int i = 0; i < trie->alphabet_size; i++) {
		if (curr->children[i] != NULL) {
			free_trie_h(trie, curr->children[i]);
		}
	}

	if (curr->ref_list) {
		ref_list_free(&curr->ref_list);
	}

	free(curr->children);
	free(curr);
}

void free_trie(trie_t **pTrie) {
	if (!pTrie || !*pTrie) {
		return;
	}
	free_trie_h(*pTrie, (*pTrie)->root);
	free(*pTrie);
	*pTrie = NULL;
} 
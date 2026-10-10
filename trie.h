/**
 * @file trie.h
 * @brief Prefix tree (Trie) for indexing and querying keywords.
 */

#ifndef TRIE_H
#define TRIE_H

struct reference_list_t;

/**
 * @brief Node in the Trie representing a character transition.
 */
typedef struct trie_node_t {
	struct reference_list_t *ref_list;
	int end_of_word;
	struct trie_node_t **children;
	int n_children;
} trie_node_t;

/**
 * @brief Trie container maintaining the root node and alphabet settings.
 */
typedef struct trie_t {
	trie_node_t *root;
	int size;
	int data_size;
	int alphabet_size;
	char *alphabet;
	int nNodes;
} trie_t;

trie_node_t *trie_create_node(trie_t *trie);
trie_t *create_trie(int data_size, int alphabet_size, char *alphabet);
void insert_key(trie_t *trie, char *key, void *value);
trie_node_t *search_key(trie_t *trie, char *key);
trie_node_t *search_prefix(trie_t *trie, char *prefix);
int delete_key(trie_t *trie, trie_node_t *curr, char *key);
void free_trie_h(trie_t *trie, trie_node_t *curr);
void free_trie(trie_t **pTrie);

#endif /* TRIE_H */
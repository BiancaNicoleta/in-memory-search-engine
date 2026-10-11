/**
 * @file list_functions.h
 * @brief Generic doubly linked list and document reference list implementations.
 */

#ifndef LIST_FUNCTIONS_H
#define LIST_FUNCTIONS_H

struct FileInfo;

/**
 * @brief Node of generic doubly linked list.
 */
typedef struct dll_node_t {
	void *data;
	struct dll_node_t *next;
	struct dll_node_t *prev;
} dll_node_t;

/**
 * @brief Generic doubly linked list container.
 */
typedef struct dll_t {
	dll_node_t *head;
	dll_node_t *tail;
	unsigned int size;
	unsigned int data_size;
} dll_t;

void dll_free_node(dll_node_t **node);
dll_t *dll_create(unsigned int data_size);
dll_node_t *dll_get_nth_node(dll_t *list, unsigned int n);
void dll_add_nth_node(dll_t *list, unsigned int n, const void *new_data);
dll_node_t *dll_remove_nth_node(dll_t *list, unsigned int n);
unsigned int dll_get_size(dll_t *list);
void dll_free(dll_t **list);

/**
 * @brief Node in a singly linked list referencing an indexed document.
 */
typedef struct reference_node_t {
	struct FileInfo *file;
	int tf;
	struct reference_node_t *next;
} reference_node_t;

/**
 * @brief List of document references associated with a keyword in the Trie.
 */
typedef struct reference_list_t {
	reference_node_t *head;
	reference_node_t *tail;
	unsigned int size;
} reference_list_t;

void ref_list_free_node(reference_node_t **node);
reference_list_t *ref_list_create(void);
void ref_list_add_nth_node(reference_list_t *list, unsigned int n, const void *new_data);
reference_node_t *ref_list_remove_nth_node(reference_list_t *list, unsigned int n);
reference_node_t *ref_list_check_node_exists(reference_list_t *list, struct FileInfo *file);
void ref_list_free(reference_list_t **list);

#endif /* LIST_FUNCTIONS_H */
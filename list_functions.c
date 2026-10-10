/**
 * @file list_functions.c
 * @brief Implementations for generic doubly linked list and document reference list.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "list_functions.h"

void dll_free_node(dll_node_t **node) {
	free((*node)->data);
	free(*node);
}

dll_t *dll_create(unsigned int data_size) {
	dll_t *list = malloc(sizeof(dll_t));
	if (list == NULL) {
		return NULL;
	}

	list->size = 0;
	list->data_size = data_size;
	list->head = NULL;
	list->tail = NULL;

	return list;
}

dll_node_t *dll_get_nth_node(dll_t *list, unsigned int n) {
	if (list == NULL || list->size == 0) {
		return NULL;
	}

	if (n >= list->size) {
		n = n % list->size;
	}

	dll_node_t *node = list->head;
	for (unsigned int i = 0; i < n; i++) {
		node = node->next;
	}

	return node;
}

void dll_add_nth_node(dll_t *list, unsigned int n, const void *new_data) {
	if (list == NULL) {
		return;
	}

	if (n > list->size) {
		n = list->size;
	}

	dll_node_t *new_node = malloc(sizeof(dll_node_t));
	if (new_node == NULL) {
		return;
	}

	new_node->data = malloc(list->data_size);
	if (new_node->data == NULL) {
		dll_free_node(&new_node);
		return;
	}
	memcpy(new_node->data, new_data, list->data_size);

	if (n == 0) {
		new_node->next = list->head;
		new_node->prev = NULL;
		if (list->head != NULL) {
			list->head->prev = new_node;
		} else {
			list->tail = new_node;
		}
		list->head = new_node;
	} else {
		dll_node_t *node = list->head;
		if (node == NULL) {
			dll_free_node(&new_node);
			return;
		}
		for (unsigned int i = 0; i < n - 1; i++) {
			node = node->next;
		}
		new_node->next = node->next;
		new_node->prev = node;
		if (node->next != NULL) {
			node->next->prev = new_node;
		} else {
			list->tail = new_node;
		}
		node->next = new_node;
	}

	list->size++;
}

dll_node_t *dll_remove_nth_node(dll_t *list, unsigned int n) {
	if (list == NULL || list->size == 0) {
		return NULL;
	}

	if (n >= list->size - 1) {
		n = list->size - 1;
	}

	dll_node_t *removed_node;

	if (n == 0) {
		removed_node = list->head;
		list->head = list->head->next;
		if (list->head != NULL) {
			list->head->prev = NULL;
		} else {
			list->tail = NULL;
		}
	} else {
		dll_node_t *node = list->head;
		for (unsigned int i = 0; i < n - 1; i++) {
			node = node->next;
		}
		removed_node = node->next;
		node->next = removed_node->next;
		if (removed_node->next != NULL) {
			removed_node->next->prev = node;
		} else {
			list->tail = node;
		}
	}

	list->size--;
	return removed_node;
}

unsigned int dll_get_size(dll_t *list) {
	if (list->head == NULL) {
		return 0;
	}

	return list->size;
}

void dll_free(dll_t **list) {
	if (list == NULL || *list == NULL) {
		return;
	}

	dll_node_t *node = (*list)->head;
	while (node != NULL) {
		dll_node_t *temp_node = node;
		node = node->next;
		dll_free_node(&temp_node);
	}

	free(*list);
	*list = NULL;
}

reference_list_t *ref_list_create(void) {
	reference_list_t *list;
	list = malloc(sizeof(reference_list_t));

	list->head = NULL;
	list->size = 0;
	return list;
}

void ref_list_free_node(reference_node_t **node) {
	free(*node);
}

void ref_list_add_nth_node(reference_list_t *list, unsigned int n,
						   const void *new_data) {
	if (list == NULL) {
		return;
	}

	if (n > list->size) {
		n = list->size;
	}

	reference_node_t *new_node = malloc(sizeof(reference_node_t));
	new_node->file = (FileInfo*)new_data;

	if (n == 0) {
		new_node->next = list->head;
		list->head = new_node;
	} else {
		reference_node_t *node = list->head;
		for (unsigned int i = 0; i < n - 1; i++) {
			node = node->next;
		}
		new_node->next = node->next;
		node->next = new_node;
	}

	list->size++;
}

reference_node_t *ref_list_remove_nth_node(reference_list_t *list,
										   unsigned int n) {
	if (list == NULL || list->size == 0)
		return NULL;

	if (n >= list->size - 1) {
		n = list->size - 1;
	}

	reference_node_t *removed_node;

	if (n == 0) {
		removed_node = list->head;
		list->head = list->head->next;
	} else {
		reference_node_t *node = list->head;
		for (unsigned int i = 0; i < n - 1; i++) {
			node = node->next;
		}
		removed_node = node->next;
		node->next = removed_node->next;
	}

	list->size--;
	return removed_node;
}

reference_node_t *ref_list_check_node_exists(reference_list_t *list,
											 FileInfo* file) {
	if (list == NULL || list->size == 0) {
		return NULL;
	}

	reference_node_t *node = list->head;
	while(node != NULL) {
		if(strcmp(node->file->id, file->id) == 0) {
			return node;
		}
		node = node->next;
	}

	return NULL;
}

void ref_list_free(reference_list_t **list) {
	if (list == NULL || *list == NULL) {
		return;
	}

	reference_node_t *node = (*list)->head;
	while (node != NULL) {
		reference_node_t *temp = node;
		node = node->next;
		ref_list_free_node(&temp);
	}

	free(*list);
	*list = NULL;
}
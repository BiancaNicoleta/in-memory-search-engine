/**
 * @file heap.h
 * @brief Generic binary heap implementation supporting custom comparators.
 */

#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include "structs.h"

#define GO_UP(x) (((x) - 1) >> 1)
#define GO_LEFT(x) (((x) << 1) + 1)
#define GO_RIGHT(x) (((x) << 1) + 2)

/**
 * @brief Generic resizable binary heap.
 */
typedef struct {
	void *arr;
	int size;
	int capacity;
	size_t data_size;
	int (*cmp)(const void *key1, const void *key2);
} heap_t;

/**
 * @brief Allocates and initializes a new heap.
 * @param cmp_f Comparator function returning positive if key1 has higher priority than key2.
 * @param data_size Size in bytes of each stored element.
 * @return Pointer to newly allocated heap_t.
 */
heap_t *create_heap(int (*cmp_f)(const void *, const void *), size_t data_size);

/**
 * @brief Inserts an element into the heap and restores heap invariant.
 */
void insert_heap(heap_t *heap, void *data);

/**
 * @brief Returns a pointer to the highest-priority element (root) without removing it.
 */
void *top(heap_t *heap);

/**
 * @brief Removes the highest-priority element and sifts down to restore heap invariant.
 */
void pop(heap_t *heap);

/**
 * @brief Checks whether the heap contains zero elements.
 */
int is_empty(heap_t *heap);

/**
 * @brief Releases heap array memory and the heap container itself.
 */
void free_heap(heap_t *heap);

#endif /* HEAP_H */

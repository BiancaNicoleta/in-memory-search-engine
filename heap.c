/**
 * @file heap.c
 * @brief Generic resizable binary heap implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heap.h"

heap_t *create_heap(int (*cmp_f)(const void *, const void *), size_t data_size)
{
	heap_t *heap = malloc(sizeof(*heap));
	DIE(heap == NULL, "heap malloc");

	heap_t heap_struct = (heap_t){
		.arr = malloc(5 * data_size),
		.capacity = 5,
		.data_size = data_size,
		.size = 0,
		.cmp = cmp_f
	};
	DIE(heap_struct.arr == NULL, "heap->arr malloc");

	*heap = heap_struct;
	return heap;
}

static void insert_heap_h(heap_t *heap, int pos)
{
	int p = GO_UP(pos);
	char *arr = heap->arr;
	size_t data_size = heap->data_size;
	char *tmp = malloc(data_size);
	DIE(tmp == NULL, "malloc tmp");

	while (pos > 0 &&
		   heap->cmp(arr + p * data_size, arr + pos * data_size) < 0) {
		memcpy(tmp, arr + pos * data_size, data_size);
		memcpy(arr + pos * data_size, arr + p * data_size, data_size);
		memcpy(arr + p * data_size, tmp, data_size);
		pos = p;
		p = GO_UP(pos);
	}
	free(tmp);
}

void insert_heap(heap_t *heap, void *data)
{
	char *arr = heap->arr;

	memcpy(&arr[heap->size * heap->data_size], data, heap->data_size);
	insert_heap_h(heap, heap->size);

	heap->size++;
	if (heap->size == heap->capacity) {
		heap->capacity *= 2;
		heap->arr = realloc(heap->arr, heap->capacity * heap->data_size);
		DIE(heap->arr == NULL, "heap->arr realloc");
	}
}

void *top(heap_t *heap)
{
	if (is_empty(heap)) {
		return NULL;
	}
	return heap->arr;
}

static void pop_h(heap_t *heap, int pos)
{
	int p = pos;
	int l = GO_LEFT(pos);
	int r = GO_RIGHT(pos);
	char *arr = heap->arr;
	size_t data_size = heap->data_size;
	char *tmp = malloc(data_size);
	DIE(tmp == NULL, "malloc tmp");

	while (l < heap->size) {
		int min_idx = p;
		if (heap->cmp(arr + min_idx * data_size, arr + l * data_size) < 0) {
			min_idx = l;
		}
		if (r < heap->size &&
			heap->cmp(arr + min_idx * data_size, arr + r * data_size) < 0) {
			min_idx = r;
		}

		if (min_idx == p) {
			break;
		}

		memcpy(tmp, arr + p * data_size, data_size);
		memcpy(arr + p * data_size, arr + min_idx * data_size, data_size);
		memcpy(arr + min_idx * data_size, tmp, data_size);

		p = min_idx;
		l = GO_LEFT(p);
		r = GO_RIGHT(p);
	}
	free(tmp);
}

void pop(heap_t *heap)
{
	char *arr = heap->arr;

	memcpy(&arr[0], &arr[(heap->size - 1) * heap->data_size], heap->data_size);
	heap->size--;

	pop_h(heap, 0);
}

int is_empty(heap_t *heap)
{
	return heap->size <= 0;
}

void free_heap(heap_t *heap)
{
	if (!heap) {
		return;
	}
	free(heap->arr);
	free(heap);
}
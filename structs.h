/**
 * @file structs.h
 * @brief Core data structures and error handling macros for the search engine.
 */

#ifndef STRUCTS_H
#define STRUCTS_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* Forward declarations */
struct dll_t;
struct trie_t;

#define DIE(assertion, call_description)  \
	do                                    \
	{                                     \
		if (assertion)                    \
		{                                 \
			fprintf(stderr, "(%s, %d): ", \
					__FILE__, __LINE__);  \
			perror(call_description);     \
			exit(errno);                  \
		}                                 \
	} while (0)

/**
 * @brief Document metadata holding identifier, relevance score, and keyword list.
 */
typedef struct FileInfo {
	char *id;
	int score;
	struct dll_t *keywords;
} FileInfo;

/**
 * @brief In-memory search engine state containing indexed documents and prefix index.
 */
typedef struct System {
	struct dll_t *files;
	struct trie_t *trie;
} System;

#endif /* STRUCTS_H */
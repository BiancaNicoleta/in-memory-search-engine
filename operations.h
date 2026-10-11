/**
 * @file operations.h
 * @brief High-level command handlers for document and index management.
 */

#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stdio.h>

struct System;
struct trie_node_t;
struct reference_list_t;

/**
 * @brief Search result item combining document metadata, term frequency, and dynamic relevance score.
 */
typedef struct SearchResult {
	FileInfo *file;
	int base_score;
	int total_tf;
	int relevance_score;
} SearchResult;

/**
 * @brief Reads a whitespace- or quote-delimited string token from a stream.
 */
char *read_string_from_file(FILE *fin);

/* --- Programmatic Core Engine API --- */

/**
 * @brief Evaluates an AND boolean query across multiple keywords.
 * @return Newly allocated reference_list_t of matching files (caller frees with ref_list_free).
 */
struct reference_list_t *search_boolean_and(struct System *system, char **keywords, int kw_count);

/**
 * @brief Evaluates an OR boolean query across multiple keywords.
 * @return Newly allocated reference_list_t of matching files (caller frees with ref_list_free).
 */
struct reference_list_t *search_boolean_or(struct System *system, char **keywords, int kw_count);

/**
 * @brief Retrieves the term frequency of a keyword within a specific document.
 */
int get_term_frequency(struct System *system, const char *doc_id, const char *keyword);

/**
 * @brief Ranks document matches by composite relevance (Base Authority + Term Frequency Weight).
 * @param list List of matching document references.
 * @param out_count Output parameter receiving the number of ranked items.
 * @return Dynamically allocated array of SearchResult items (caller frees).
 */
SearchResult *rank_results_by_tf(struct reference_list_t *list, int *out_count);

/**
 * @brief Programmatically indexes a document with an ID, score, and array of keywords.
 * @return 1 on success, 0 if document ID already exists.
 */
int add_document(struct System *system, const char *id, int score, int kw_count, char **keywords);

/**
 * @brief Programmatically removes a document by ID.
 * @return 1 if found and removed, 0 if not found.
 */
int delete_document(struct System *system, const char *id);

/**
 * @brief Programmatically adds a keyword to an existing document.
 * @return 1 on success, 0 if document not found.
 */
int add_keyword_to_doc(struct System *system, const char *id, const char *keyword);

/**
 * @brief Programmatically removes a keyword from an existing document.
 * @return 1 on success, 0 if document not found.
 */
int delete_keyword_from_doc(struct System *system, const char *id, const char *keyword);

/**
 * @brief Finds a document by its ID.
 */
FileInfo *get_file_by_id(struct System *system, const char *id, int *out_index);

/**
 * @brief Finds index of a keyword inside a document's keyword list.
 */
int get_keyword_index(FileInfo *file, const char *keyword);

/**
 * @brief Comparator function for ranking files in max-heap (by score descending, ID ascending).
 */
int cmp_files(const void *a, const void *b);

/**
 * @brief String comparison helper for qsort on arrays of string pointers.
 */
int cmp_str(const void *a, const void *b);

/**
 * @brief Recursively gathers document references matching a prefix in the Trie.
 */
void collect_prefix_references(struct System *system, struct trie_node_t *curr, struct reference_list_t *list);

/* --- Batch Stream Command Handlers --- */

/**
 * @brief Adds a new document with score and keywords to the search index (batch stream).
 */
void add(struct System *system, FILE *fin, FILE *fout);

/**
 * @brief Removes a document by ID and cleans up keyword references (batch stream).
 */
void del(struct System *system, FILE *fin, FILE *fout);

/**
 * @brief Associates an additional keyword with an existing document (batch stream).
 */
void addkw(struct System *system, FILE *fin, FILE *fout);

/**
 * @brief Disassociates a keyword from an existing document (batch stream).
 */
void delkw(struct System *system, FILE *fin, FILE *fout);

/**
 * @brief Searches for a keyword and prints alphabetically sorted matching document IDs.
 */
void find(struct System *system, FILE *fin, FILE *fout);

/**
 * @brief Retrieves top-K documents for a keyword ranked by score and ID.
 */
void topk(struct System *system, FILE *fin, FILE *fout);

/**
 * @brief Prints the entire inverted index dictionary and referencing documents.
 */
void print(struct System *system, FILE *fout);

/**
 * @brief Queries documents having any keyword beginning with the given prefix.
 */
void prefix(struct System *system, FILE *fin, FILE *fout);

#endif /* OPERATIONS_H */
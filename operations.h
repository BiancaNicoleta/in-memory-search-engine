/**
 * @file operations.h
 * @brief High-level command handlers for document and index management.
 */

#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stdio.h>

struct System;

/**
 * @brief Reads a whitespace- or quote-delimited string token from a stream.
 */
char *read_string_from_file(FILE *fin);

/**
 * @brief Adds a new document with score and keywords to the search index.
 */
void add(struct System *system, FILE *fin, FILE *fout);

/**
 * @brief Removes a document by ID and cleans up keyword references.
 */
void del(struct System *system, FILE *fin, FILE *fout);

/**
 * @brief Associates an additional keyword with an existing document.
 */
void addkw(struct System *system, FILE *fin, FILE *fout);

/**
 * @brief Disassociates a keyword from an existing document.
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
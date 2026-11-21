/*
 * index.h - Public API for building and querying an inverted index.
 *
 * PURPOSE
 *   Maps a normalized term to the IDs of documents containing that term.
 *   The index lets searches avoid rescanning every document.
 *
 * TYPICAL USE
 *   InvertedIndex index;
 *   init_index(&index);
 *
 *   if (build_index(&index, &collection))
 *   {
 *       search_index(&index, "graphics");
 *   }
 *
 *   free_index(&index);   // REQUIRED when finished
 */

#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include "document.h"

/*
 * IndexEntry - Posting list for one term.
 *
 * MEMORY OWNERSHIP
 *   term and document_ids are allocated by build_index() and released by
 *   free_index(). Callers must not free them individually.
 */
typedef struct
{
    char *term;                    /* Normalized lowercase search term. */
    int *document_ids;             /* IDs of documents containing the term. */
    size_t document_count;         /* Number of valid IDs in document_ids[]. */
    size_t document_capacity;      /* Allocated capacity of document_ids[]. */
} IndexEntry;

/*
 * InvertedIndex - Dynamically allocated array of IndexEntry objects.
 *
 * MEMORY OWNERSHIP
 *   entries and all entry internals are allocated by build_index() and must be
 *   released with free_index().
 */
typedef struct
{
    IndexEntry *entries;           /* Array of term entries. */
    size_t count;                  /* Number of term entries. */
    size_t capacity;               /* Allocated capacity of entries[]. */
} InvertedIndex;

/*
 * init_index
 * ----------
 * WHAT IT DOES
 *   Initialises an index to empty (NULL, zero count, zero capacity).
 *
 * PARAMETERS
 *   index : Index to initialise. May be NULL.
 *
 * RETURNS
 *   Nothing.
 *
 * MEMORY
 *   No allocation. Call before build_index(). Release with free_index().
 *
 * EXAMPLE
 *   InvertedIndex index;
 *   init_index(&index);
 */
void init_index(InvertedIndex *index);

/*
 * build_index
 * -----------
 * WHAT IT DOES
 *   Scans every document's text, normalizes words to lowercase alphanumeric
 *   terms, and adds each document ID to the posting list for each term.
 *
 * PARAMETERS
 *   index      : Initialised InvertedIndex to fill.
 *   collection : Loaded Collection to index. Its texts must be present.
 *
 * RETURNS
 *   1 on success, 0 on invalid arguments or allocation failure.
 *
 * OUTPUT
 *   Fills index->entries with one IndexEntry per unique term and each entry's
 *   document_ids array. Duplicate document IDs for the same term are not added
 *   twice. Does not print output.
 *
 * MEMORY
 *   Allocates index entries, term strings, and posting lists. The caller MUST
 *   call free_index(index) when finished. Freeing one entry directly is unsafe.
 *
 * EXAMPLE
 *   InvertedIndex index;
 *   init_index(&index);
 *   if (!build_index(&index, &collection)) { handle the error in your code }
 *   free_index(&index);
 */
int build_index(
    InvertedIndex *index,
    const Collection *collection
);

/*
 * search_index
 * ------------
 * WHAT IT DOES
 *   Normalizes one query term, looks it up in the inverted index, and prints
 *   the IDs of all documents containing that term.
 *
 * PARAMETERS
 *   index : Built InvertedIndex to query. May be NULL.
 *   query : Null-terminated query string. May contain uppercase/punctuation;
 *           it is normalized internally. This version searches one combined
 *           alphanumeric term, so it is intended for single-term queries.
 *
 * RETURNS
 *   Nothing.
 *
 * OUTPUT
 *   Prints either:
 *       Documents containing '<term>':
 *       Document ID: <id>
 *   or an explanatory message when the query is invalid or has no matches.
 *
 * MEMORY
 *   Allocates a temporary normalized query buffer and frees it internally.
 *   Nothing for the caller to free.
 *
 * EXAMPLE
 *   search_index(&index, "graphics");
 */
void search_index(
    const InvertedIndex *index,
    const char *query
);

/*
 * free_index
 * ----------
 * WHAT IT DOES
 *   Frees every term string, every posting list, the entries array, and resets
 *   the index to empty.
 *
 * PARAMETERS
 *   index : InvertedIndex to release. May be NULL.
 *
 * RETURNS
 *   Nothing.
 *
 * MEMORY
 *   REQUIRED after build_index()/successful indexing. Do not use any
 *   IndexEntry or term pointer after this call.
 *
 * EXAMPLE
 *   free_index(&index);
 */
void free_index(InvertedIndex *index);

#endif

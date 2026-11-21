
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "index.h"


/*
 * find_term - INTERNAL HELPER (static)
 * ------------------------------------
 * Input:
 *   index - built inverted index; must be non-NULL.
 *   term  - normalized lowercase term to find.
 *
 * Action:
 *   Linear search through index->entries[] comparing term strings.
 *
 * Expected output:
 *   Returns the matching entry index, or -1 when not present.
 *
 * Memory:
 *   No allocation and nothing to free.
 */
static int find_term(const InvertedIndex *index, const char *term)
{
    for (size_t i = 0; i < index->count; i++)
    {
        if (strcmp(index->entries[i].term, term) == 0)
        {
            return (int)i;
        }
    }

    return -1;
}


/*
 * add_document_id - INTERNAL HELPER (static)
 * ------------------------------------------
 * Input:
 *   entry       - IndexEntry receiving a document ID.
 *   document_id - ID to append if not already present.
 *
 * Action:
 *   Avoids duplicates, grows entry->document_ids when necessary, and appends.
 *
 * Expected output:
 *   Returns 1 on success or 0 on allocation failure.
 *
 * Memory:
 *   May reallocate entry->document_ids; the entry owns it and free_index()
 *   releases it later.
 */
static int add_document_id(IndexEntry *entry, int document_id)
{
    for (size_t i = 0; i < entry->document_count; i++)
    {
        if (entry->document_ids[i] == document_id)
        {
            return 1;
        }
    }

    if (entry->document_count == entry->document_capacity)
    {
        size_t new_capacity =
            (entry->document_capacity == 0)
            ? 4
            : entry->document_capacity * 2;

        int *new_ids = realloc(
            entry->document_ids,
            new_capacity * sizeof(int)
        );

        if (new_ids == NULL)
        {
            return 0;
        }

        entry->document_ids = new_ids;
        entry->document_capacity = new_capacity;
    }

    entry->document_ids[entry->document_count] = document_id;
    entry->document_count++;

    return 1;
}


/*
 * add_term - INTERNAL HELPER (static)
 * -----------------------------------
 * Input:
 *   index       - InvertedIndex being built.
 *   term        - normalized lowercase term to add.
 *   document_id - document containing the term.
 *
 * Action:
 *   Reuses an existing IndexEntry or creates a new one, then adds the document ID.
 *
 * Expected output:
 *   Returns 1 on success or 0 on allocation failure.
 *
 * Memory:
 *   May allocate an entry, its term string, and its posting list. All are
 *   owned by the index and released by free_index().
 */
static int add_term(
    InvertedIndex *index,
    const char *term,
    int document_id
)
{
    int position = find_term(index, term);

    if (position >= 0)
    {
        return add_document_id(
            &index->entries[position],
            document_id
        );
    }

    if (index->count == index->capacity)
    {
        size_t new_capacity =
            (index->capacity == 0)
            ? 16
            : index->capacity * 2;

        IndexEntry *new_entries = realloc(
            index->entries,
            new_capacity * sizeof(IndexEntry)
        );

        if (new_entries == NULL)
        {
            return 0;
        }

        index->entries = new_entries;
        index->capacity = new_capacity;
    }

    IndexEntry *entry = &index->entries[index->count];

    entry->term = malloc(strlen(term) + 1);

    if (entry->term == NULL)
    {
        return 0;
    }

    strcpy(entry->term, term);

    entry->document_ids = NULL;
    entry->document_count = 0;
    entry->document_capacity = 0;

    if (!add_document_id(entry, document_id))
    {
        free(entry->term);
        entry->term = NULL;
        return 0;
    }

    index->count++;

    return 1;
}


/*
 * init_index - PUBLIC API
 * -----------------------
 * Input:
 *   my_index - InvertedIndex to reset; may be NULL.
 *
 * Action:
 *   Sets entries to NULL and count/capacity to 0.
 *
 * Expected output:
 *   None. The index is ready for build_index().
 *
 * Memory:
 *   No allocation. Call free_index() after the index has been built.
 */
void init_index(InvertedIndex *my_index)
{
    my_index->entries = NULL;
    my_index->count = 0;
    my_index->capacity = 0;
}


/*
 * build_index - PUBLIC API
 * ------------------------
 * Input:
 *   index      - initialised InvertedIndex to fill.
 *   collection - Collection whose documents have loaded text buffers.
 *
 * Action:
 *   For every document, scans its text into lowercase alphanumeric terms and
 *   adds the document ID to each term's posting list.
 *
 * Expected output:
 *   Returns 1 on success, 0 on invalid input/allocation failure. Fills
 *   index->entries with unique terms and their document ID lists. Prints nothing.
 *
 * Memory:
 *   Allocates entries, term strings, and posting lists. The caller MUST call
 *   free_index(index) when finished; do not free individual entries.
 *
 * Example:
 *   InvertedIndex index;
 *   init_index(&index);
 *   if (build_index(&index, &collection)) { search_index(&index, "graphics"); }
 *   free_index(&index);
 */
int build_index(
    InvertedIndex *index,
    const Collection *collection
)
{
    if (index == NULL || collection == NULL)
    {
        return 0;
    }

    for (size_t i = 0; i < collection->count; i++)
    {
        const Document *doc = &collection->documents[i];

        if (doc->text == NULL)
        {
            continue;
        }

        size_t length = strlen(doc->text);

        char *text_copy = malloc(length + 1);

        if (text_copy == NULL)
        {
            return 0;
        }

        strcpy(text_copy, doc->text);

        size_t start = 0;

        for (size_t j = 0; j <= length; j++)
        {
            unsigned char ch = (unsigned char)text_copy[j];

            if (isalnum(ch))
            {
                text_copy[j] = (char)tolower(ch);
            }
            else
            {
                if (j > start)
                {
                    char saved_char = text_copy[j];
                    text_copy[j] = '\0';

                    if (!add_term(index, text_copy + start, doc->id))
                    {
                        free(text_copy);
                        return 0;
                    }

                    text_copy[j] = saved_char;
                }

                start = j + 1;
            }
        }

        free(text_copy);
    }

    return 1;
}


/*
 * search_index - PUBLIC API
 * -------------------------
 * Input:
 *   index - built InvertedIndex to query; NULL prints an error.
 *   query - query string to normalize and look up; NULL/empty prints an error.
 *
 * Action:
 *   Removes non-alphanumeric characters, converts the query to lowercase,
 *   finds the matching IndexEntry, and prints its document IDs.
 *
 * Expected output:
 *   Prints "Documents containing '<term>':" followed by one
 *   "Document ID: <id>" line per match. Prints an explanatory message when the
 *   query is invalid or no entry exists. Returns nothing.
 *
 * Memory:
 *   Allocates a temporary normalized-query buffer and frees it internally.
 *   Nothing for the caller to free.
 */
void search_index(
    const InvertedIndex *index,
    const char *query
)
{
    if (index == NULL || query == NULL || query[0] == '\0')
    {
        printf("Invalid search query.\n");
        return;
    }

    size_t length = strlen(query);

    char *normalised_query = malloc(length + 1);

    if (normalised_query == NULL)
    {
        printf("Memory allocation failed.\n");
        return;
    }

    size_t position = 0;

    for (size_t i = 0; i < length; i++)
    {
        unsigned char ch = (unsigned char)query[i];

        if (isalnum(ch))
        {
            normalised_query[position++] = (char)tolower(ch);
        }
    }

    normalised_query[position] = '\0';

    if (position == 0)
    {
        printf("No valid search term.\n");
        free(normalised_query);
        return;
    }

    int term_position = find_term(index, normalised_query);

    if (term_position < 0)
    {
        printf("No documents found for '%s'.\n", normalised_query);
        free(normalised_query);
        return;
    }

    const IndexEntry *entry = &index->entries[term_position];

    printf("Documents containing '%s':\n", normalised_query);

    for (size_t i = 0; i < entry->document_count; i++)
    {
        printf("Document ID: %d\n", entry->document_ids[i]);
    }

    free(normalised_query);
}


/*
 * free_index - PUBLIC API
 * -----------------------
 * Input:
 *   my_index - InvertedIndex to release; may be NULL.
 *
 * Action:
 *   Frees every entry's term string and posting list, then frees the entries
 *   array and resets the index to NULL/0/0.
 *
 * Expected output:
 *   None. No IndexEntry pointer is valid after this call.
 *
 * Memory:
 *   REQUIRED cleanup for build_index(). Do not free entries individually.
 */
void free_index(InvertedIndex *my_index)
{
    if (my_index == NULL)
    {
        return;
    }

    for (size_t i = 0; i < my_index->count; i++)
    {
        free(my_index->entries[i].term);
        free(my_index->entries[i].document_ids);
    }

    free(my_index->entries);

    my_index->entries = NULL;
    my_index->count = 0;
    my_index->capacity = 0;
}

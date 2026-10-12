
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "index.h"


/* Find a term in the inverted index */
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


/* Add a document ID to an index entry */
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


/* Add a term and its document ID to the inverted index */
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


/* Initialize the inverted index */
void init_index(InvertedIndex *my_index)
{
    my_index->entries = NULL;
    my_index->count = 0;
    my_index->capacity = 0;
}


/* Build the inverted index from a collection of documents */
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


/* Search the inverted index */
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


/* Free the memory used by the inverted index */
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

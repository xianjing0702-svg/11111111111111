
#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <stddef.h>

#define MAX_FILENAME 255

typedef struct
{
    int id;
    char filename[MAX_FILENAME + 1];
    char *text;
    size_t text_length;
    long total_terms;
    long unique_terms;
} Document;

typedef struct
{
    Document *documents;
    size_t count;
    size_t capacity;
} Collection;

/* Collection management */
void init_collection(Collection *collection);
void free_collection(Collection *collection);

/* Load documents */
int load_collection(
    Collection *collection,
    const char *manifest_file,
    const char *documents_folder
);

/* Document utilities */
const Document *get_document(
    const Collection *collection,
    int document_id
);

void print_document(
    const Collection *collection,
    int document_id
);

void print_collection_statistics(
    const Collection *collection
);

#endif

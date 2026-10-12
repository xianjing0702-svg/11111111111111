/* This function is to define index.h, for main.c to call it. */
#ifndef INDEX_H
#define INDEX_H

#include "document.h"
#include <stdlib.h>

typedef struct 
{
    char *term;
    int *document_ids;       // 之前漏掉了这个指针，导致 index.c 找不到这个变量
    size_t document_count;
    size_t document_capacity;
} IndexEntry;                // 统一改成首字母大写的 IndexEntry

typedef struct 
{
    IndexEntry *entries;
    size_t count;            // 必须改成 count，因为 index.c 里用的是 index->count
    size_t capacity;         // 必须改成 capacity，因为 index.c 里用的是 index->capacity
} InvertedIndex;             // 统一改成首字母大写的 InvertedIndex

void init_index(InvertedIndex *index);
int  build_index(InvertedIndex *index, const Collection *collection);
void search_index(const InvertedIndex *index, const char *query);
void free_index(InvertedIndex *index);

#endif




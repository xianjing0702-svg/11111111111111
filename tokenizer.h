
#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include "document.h"

typedef struct
{
    char **tokens;
    size_t count;
    size_t capacity;
} TokenList;

/* Initialise and free a token list */
void init_token_list(TokenList *list);
void free_token_list(TokenList *list);

/* Split text into lowercase tokens */
int tokenize_text(const char *text, TokenList *list);

/* Calculate the term statistics of a document */
int process_document(Document *document);

/* Process every document in a collection */
int process_collection(Collection *collection);

#endif

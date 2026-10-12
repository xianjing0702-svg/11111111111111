
#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Initialise an empty token list */
void init_token_list(TokenList *list)
{
    if (list == NULL)
    {
        return;
    }

    list->tokens = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* Free all tokens and the token array */
void free_token_list(TokenList *list)
{
    size_t i;

    if (list == NULL)
    {
        return;
    }

    for (i = 0; i < list->count; i++)
    {
        free(list->tokens[i]);
    }

    free(list->tokens);

    list->tokens = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* Add a token to the token list */
static int add_token(
    TokenList *list,
    const char *start,
    size_t length
)
{
    char *token;
    size_t i;

    if (length == 0)
    {
        return 1;
    }

    /* Allocate memory for the token */
    token = malloc(length + 1);

    if (token == NULL)
    {
        return 0;
    }

    /* Convert the token to lowercase */
    for (i = 0; i < length; i++)
    {
        token[i] = (char)tolower((unsigned char)start[i]);
    }

    token[length] = '\0';

    /* Expand the array if it is full */
    if (list->count == list->capacity)
    {
        size_t new_capacity;
        char **new_tokens;

        if (list->capacity == 0)
        {
            new_capacity = 16;
        }
        else
        {
            if (list->capacity > (size_t)-1 / 2)
            {
                free(token);
                return 0;
            }

            new_capacity = list->capacity * 2;
        }

        if (new_capacity > (size_t)-1 / sizeof(char *))
        {
            free(token);
            return 0;
        }

        new_tokens = realloc(
            list->tokens,
            new_capacity * sizeof(char *)
        );

        if (new_tokens == NULL)
        {
            free(token);
            return 0;
        }

        list->tokens = new_tokens;
        list->capacity = new_capacity;
    }

    list->tokens[list->count] = token;
    list->count++;

    return 1;
}

/* Split text into lowercase words */
int tokenize_text(const char *text, TokenList *list)
{
    const char *start;
    const char *current;

    if (text == NULL || list == NULL)
    {
        return 0;
    }

    init_token_list(list);

    current = text;

    while (*current != '\0')
    {
        /* Skip whitespace and punctuation */
        while (*current != '\0' &&
               !isalnum((unsigned char)*current))
        {
            current++;
        }

        if (*current == '\0')
        {
            break;
        }

        start = current;

        /* Read one word */
        while (*current != '\0' &&
               isalnum((unsigned char)*current))
        {
            current++;
        }

        /* Save the word */
        if (!add_token(
                list,
                start,
                (size_t)(current - start)))
        {
            free_token_list(list);
            return 0;
        }
    }

    return 1;
}

/* Calculate the number of unique tokens */
static long count_unique_tokens(const TokenList *list)
{
    size_t i;
    size_t j;
    long unique_count = 0;

    for (i = 0; i < list->count; i++)
    {
        int found = 0;

        /* Check whether this word appeared before */
        for (j = 0; j < i; j++)
        {
            if (strcmp(
                    list->tokens[i],
                    list->tokens[j]) == 0)
            {
                found = 1;
                break;
            }
        }

        if (!found)
        {
            unique_count++;
        }
    }

    return unique_count;
}

/* Process one document */
int process_document(Document *document)
{
    TokenList list;

    if (document == NULL || document->text == NULL)
    {
        return 0;
    }

    if (!tokenize_text(document->text, &list))
    {
        return 0;
    }

    document->total_terms = (long)list.count;

    document->unique_terms = count_unique_tokens(&list);

    free_token_list(&list);

    return 1;
}

/* Process every document in a collection */
int process_collection(Collection *collection)
{
    size_t i;

    if (collection == NULL)
    {
        return 0;
    }

    for (i = 0; i < collection->count; i++)
    {
        if (!process_document(&collection->documents[i]))
        {
            fprintf(
                stderr,
                "Failed to process document ID %d\n",
                collection->documents[i].id
            );

            return 0;
        }
    }

    return 1;
}

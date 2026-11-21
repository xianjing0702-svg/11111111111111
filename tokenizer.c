
#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * init_token_list - PUBLIC API
 * ---------------------------
 * Input:
 *   list - TokenList to reset; NULL is safe.
 *
 * Action:
 *   Sets tokens to NULL, count to 0, and capacity to 0.
 *
 * Expected output:
 *   None. list is ready for tokenize_text().
 *
 * Memory:
 *   No allocation. If list already owns tokens, call free_token_list() first
 *   to avoid losing that previous allocation.
 */
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

/*
 * free_token_list - PUBLIC API
 * ---------------------------
 * Input:
 *   list - TokenList whose owned strings and array should be released; NULL is safe.
 *
 * Action:
 *   Frees list->tokens[i] for every i < count, then frees list->tokens.
 *
 * Expected output:
 *   None. The list is reset to NULL/0/0 and must not be read afterwards.
 *
 * Memory:
 *   REQUIRED after successful tokenize_text(). Safe on an empty list.
 */
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

/*
 * add_token - INTERNAL HELPER (static)
 * -----------------------------------
 * Input:
 *   list   - destination TokenList.
 *   start  - pointer to the first character of a word.
 *   length - number of characters in that word.
 *
 * Action:
 *   Allocates a new lowercase word, grows list->tokens if needed, and appends it.
 *
 * Expected output:
 *   Returns 1 on success, or 0 on allocation/capacity failure. Nothing printed.
 *
 * Memory:
 *   Allocates the token and possibly a larger token array. Both are released by
 *   free_token_list().
 */
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

/*
 * tokenize_text - PUBLIC API
 * -------------------------
 * Input:
 *   text - null-terminated text to split; NULL returns 0.
 *   list - caller-owned TokenList that receives lowercase words.
 *
 * Action:
 *   Skips non-alphanumeric separators, copies each alphanumeric run, converts it
 *   to lowercase, and appends it to list.
 *
 * Expected output:
 *   Returns 1 on success, 0 on invalid input/allocation failure. On success,
 *   list->tokens[0..count-1] contains newly allocated lowercase words.
 *
 * Memory:
 *   Reinitialises list and allocates all token strings/array. The caller MUST
 *   call free_token_list(list) after success. On failure partial memory is freed.
 *
 * Example:
 *   TokenList words;
 *   if (tokenize_text("Hello, HELLO world!", &words))
 *   {
 *       // count == 3, tokens: "hello", "hello", "world"
 *       free_token_list(&words);
 *   }
 */
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

/*
 * count_unique_tokens - INTERNAL HELPER (static)
 * ---------------------------------------------
 * Input:
 *   list - token list to inspect.
 *
 * Action:
 *   Counts distinct strings using a straightforward O(n^2) comparison.
 *
 * Expected output:
 *   Returns the number of different tokens in the list.
 *
 * Memory:
 *   No allocation and nothing to free.
 */
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

/*
 * process_document - PUBLIC API
 * -----------------------------
 * Input:
 *   document - Document with a loaded text buffer; NULL/invalid text returns 0.
 *
 * Action:
 *   Tokenizes document->text and calculates total and unique term counts.
 *
 * Expected output:
 *   Returns 1 on success, 0 on failure. Updates document->total_terms and
 *   document->unique_terms. Nothing is printed.
 *
 * Memory:
 *   Uses a temporary TokenList that is freed internally. No caller cleanup.
 */
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

/*
 * process_collection - PUBLIC API
 * -------------------------------
 * Input:
 *   collection - loaded Collection to process; NULL returns 0.
 *
 * Action:
 *   Calls process_document() for every element.
 *
 * Expected output:
 *   Returns 1 when all documents succeed, otherwise 0. On failure prints the
 *   failing document ID to stderr.
 *
 * Memory:
 *   Temporary token lists are freed internally. No caller cleanup.
 */
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

/*
 * tokenizer.h - Public API for text tokenization and term statistics.
 *
 * PURPOSE
 *   Converts a string into a dynamically allocated TokenList of lowercase
 *   alphanumeric words. It is used by document processing, inverted indexing,
 *   searching, and benchmarking so those modules normalize text consistently.
 *
 * TYPICAL USE
 *   TokenList words;
 *   if (tokenize_text("Hello, world!", &words))
 *   {
 *       for (size_t i = 0; i < words.count; i++)
 *       {
 *           printf("%s\n", words.tokens[i]);
 *       }
 *       free_token_list(&words);   // REQUIRED after successful tokenization
 *   }
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include "document.h"

/*
 * TokenList - Dynamically allocated list of lowercase word strings.
 *
 * MEMORY OWNERSHIP
 *   tokenize_text() allocates every tokens[i] string and the tokens array.
 *   The caller must call free_token_list() when finished.
 */
typedef struct
{
    char **tokens;   /* Array of null-terminated lowercase words. */
    size_t count;    /* Number of valid words in tokens[]. */
    size_t capacity; /* Allocated capacity of tokens[]. */
} TokenList;

/*
 * init_token_list
 * ---------------
 * WHAT IT DOES
 *   Resets a TokenList to empty (NULL, zero count, zero capacity).
 *
 * PARAMETERS
 *   list : TokenList to initialise. May be NULL.
 *
 * RETURNS
 *   Nothing.
 *
 * MEMORY
 *   No allocation. If the list already owns tokens, call free_token_list()
 *   first; otherwise its previous allocation would be lost.
 *
 * EXAMPLE
 *   TokenList list;
 *   init_token_list(&list);
 */
void init_token_list(TokenList *list);

/*
 * free_token_list
 * ---------------
 * WHAT IT DOES
 *   Frees every allocated token string, frees the tokens array, and resets the
 *   list to empty.
 *
 * PARAMETERS
 *   list : TokenList to release. May be NULL.
 *
 * RETURNS
 *   Nothing.
 *
 * MEMORY
 *   REQUIRED after successful tokenize_text(). Do not access list.tokens or
 *   any tokens[i] after this call. Safe to call on an empty list.
 *
 * EXAMPLE
 *   free_token_list(&list);
 */
void free_token_list(TokenList *list);

/*
 * tokenize_text
 * -------------
 * WHAT IT DOES
 *   Scans text and extracts maximal runs of alphanumeric characters. Each run
 *   is copied, converted to lowercase, and appended to list. Non-alphanumeric
 *   characters act as separators, so punctuation and whitespace are ignored.
 *
 * PARAMETERS
 *   text : Null-terminated input text. May be NULL.
 *   list : TokenList that receives the words. tokenize_text() initializes it.
 *
 * RETURNS
 *   1 on success, 0 on invalid arguments or allocation failure.
 *
 * OUTPUT
 *   On success, list->tokens[0..list->count-1] contain newly allocated
 *   lowercase words. The function does not print output.
 *
 * MEMORY
 *   Allocates all token strings and the token array. The caller MUST call
 *   free_token_list(list) after success. On failure the function releases any
 *   partial allocation and returns 0.
 *
 * EXAMPLE
 *   TokenList words;
 *   if (tokenize_text("Hello, HELLO world!", &words))
 *   {
 *       // words.tokens[0] == "hello", count == 3
 *       free_token_list(&words);
 *   }
 */
int tokenize_text(const char *text, TokenList *list);

/*
 * process_document
 * ----------------
 * WHAT IT DOES
 *   Tokenizes document->text and writes the total and unique term counts into
 *   the Document.
 *
 * PARAMETERS
 *   document : Document whose text is already loaded. May be NULL.
 *
 * RETURNS
 *   1 on success, 0 for NULL/invalid input or tokenization failure.
 *
 * OUTPUT
 *   Updates:
 *       document->total_terms  = number of words
 *       document->unique_terms = number of distinct lowercase words
 *   Does not print output.
 *
 * MEMORY
 *   Uses a temporary TokenList that is freed internally. Nothing is returned
 *   for the caller to free.
 *
 * EXAMPLE
 *   Document *doc = &collection.documents[0];
 *   if (process_document(doc)) { printf("%ld\n", doc->total_terms); }
 */
int process_document(Document *document);

/*
 * process_collection
 * ------------------
 * WHAT IT DOES
 *   Calls process_document() for every document in the Collection.
 *
 * PARAMETERS
 *   collection : Collection to process. May be NULL.
 *
 * RETURNS
 *   1 if every document was processed successfully, otherwise 0.
 *
 * OUTPUT
 *   On failure, prints an error naming the failing document ID to stderr.
 *   On success, fills total_terms and unique_terms for each document.
 *
 * MEMORY
 *   Temporary token lists are freed internally. Nothing for the caller to free.
 *
 * EXAMPLE
 *   if (!process_collection(&collection)) { handle the error in your code }
 */
int process_collection(Collection *collection);

#endif

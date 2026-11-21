#ifndef SEARCH_H
#define SEARCH_H

#include "document.h"
#include "tokenizer.h"
#include <stdbool.h>

typedef struct{
    const Document *document;  // pointing to existing document from document.h
    TokenList tokens;  // tokenized text that will be initialized within load_document_linear_search function
    int matched;  // number of exact match of words
    int score; // currently not used
}SearchDocData;

typedef struct{
    TokenList tokens;  // tokenized text of user input , initialized from load_userinput_linear_search
    int *weight;  // currently not used
}SearchInpData;

// Initialized documents return true on success and false otherwise
// Take 2 parameter, collection and doc_data to initialize all data
// Typically used by passing in an array of SearchDocData to load all of its documents in collection 
bool load_documents_linear_search(const Collection *col, SearchDocData *doc_data);

// Initialize user input and tokenized it, return true on success and false otherwise
// Take 2 parameter, String of user input and SearchInpData to initialize it and store its tokens
bool load_userinput_linear_search(const char *input, SearchInpData *data);

// Must call at the end of the program to cleanup every data that have been created
// Take 3 parameter, collection to determine number of document to be clean and SearchDocData and SearchInpData to cleanup
void clean_linear_search_data(const Collection *col, SearchDocData *doc_data, SearchInpData *inp_data);

// Run comparison between user input and bufferd text in the documents
// Return true if the search succeed and false otherwise
// If a match is found, match in SearchDocData will be increament on every word found
bool linear_search(SearchDocData *doc, SearchInpData *inp, size_t index);

#endif
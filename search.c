#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "search.h"
#include "tokenizer.h"

void compare(SearchDocData *source,SearchInpData *target ){
    if(source == NULL || target == NULL)
        return;
    for(int i=0; i < source->tokens.count ; i++){
        for(int j=0; j < target->tokens.count; j++){
            if(strcmp(source->tokens.tokens[i],target->tokens.tokens[j]) == 0){
                source->matched++;
                source->score++;
            }  
        }
    }
}

bool load_documents_linear_search(const Collection *col, SearchDocData *doc_data){
    if(col == NULL || doc_data == NULL)
        return false;
    for(int i=0 ; i < col->count; i++){
        doc_data[i].document = NULL;
        doc_data[i].matched = 0;
        doc_data[i].score = 0;
        init_token_list(&doc_data[i].tokens);
        const Document *doc = get_document(col,i+1);
        if(doc == NULL){
            fprintf(stderr,"Error, NULL pointer in document id:%d\n",i);
            doc_data[i].document = NULL;
            continue;
        }
        doc_data[i].document = doc;
        init_token_list(&doc_data[i].tokens);    // need to free after use;
        if(!tokenize_text(doc->text,&doc_data[i].tokens)){
            fprintf(stderr,"Error, failed tokenize document id:%d\n",i);
            doc_data[i].document = NULL;
            free_token_list(&doc_data[i].tokens);
            continue;
        }
    }
    return true;
}

bool load_userinput_linear_search(const char *input, SearchInpData *data){
    if(input == NULL || data == NULL)
        return false;
    init_token_list(&data->tokens); // free after use
    if(!tokenize_text(input,&data->tokens)){
        free_token_list(&data->tokens);
        return false;
    }
    return true;
}

void clean_linear_search_data(const Collection *col, SearchDocData *doc_data, SearchInpData *inp_data){
    if(col == NULL || doc_data == NULL || inp_data == NULL){
        fprintf(stderr,"NULL pointer clean_linear_search\n");
        return;
    }
    for(int i=0; i < col->count; i++){
            free_token_list(&doc_data[i].tokens);
    }
    free_token_list(&inp_data->tokens);
}

bool linear_search(SearchDocData *doc, SearchInpData *inp, size_t index){
    if(doc==NULL || inp == NULL)
        return false;
    for(int i=0; i < index; i++){
        compare(&doc[i],inp);
    }
    return true;
}
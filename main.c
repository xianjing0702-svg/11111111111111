#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "index.h"
#include "document.h"
#include "search.h"

char* take_user_input();
void bubble_sort(SearchDocData data[], size_t count);

int main(void){
    Collection collection;
    init_collection(&collection);
    load_collection(&collection,"manifest.txt","documents");
    char *input = take_user_input();
    int select = 0;
    do{
        printf("1.Inverted Index\n");
        printf("2.Linear Search\n");
        printf("3.Inverted Index & Linear Search\n");
        printf("Select which search method:\n");
        scanf("%d",&select);
        if(select < 1 || select > 3)
            printf("Invalid Input.\n\n");
    }while(select < 1 || select > 3);

    time_t startI,startL,endI,endL;

    // Initialize index
    InvertedIndex index;
    init_index(&index);
    if(!build_index(&index,&collection)){
        fprintf(stderr,"Failed to build inverted index\n");
        return 1;
    }

    // Initialize linear search
    SearchDocData doc_data[collection.count];
    SearchInpData inp_data;
    if(!(load_documents_linear_search(&collection,doc_data) && load_userinput_linear_search(input,&inp_data))){
        fprintf(stderr,"Failed to build linear search\n");
        return 1;
    }

    switch(select){
        case 1:{
            startI = clock();
            search_index(&index,input);
            endI = clock();
            printf("Time used Inverted Index: %lf\n",((double)(endI)-startI)/CLOCKS_PER_SEC);
            break;
        }
        case 2:{
            startL = clock();
            linear_search(doc_data,&inp_data,collection.count);
            endL = clock();
            printf("Time used Linear Search: %lf\n",((double)(endL)-startL)/CLOCKS_PER_SEC);
            bubble_sort(doc_data,collection.count);
            for(int i=0; i < 1001; i++){
                printf("%d",doc_data[i].score);
            }
            puts("");
            for(int i=0; i < 5; i++){
                printf("id: %d, score: %d\n",doc_data[i].document->id,doc_data[i].score);
            }
            break;
        }
        case 3:{
            startI = clock();
            search_index(&index,input);
            endI  = clock();

            startL = clock();
            linear_search(doc_data,&inp_data,collection.count);
            endL = clock();

            printf("Time used Inverted Index: %lf\n",((double)(endI)-startI)/CLOCKS_PER_SEC);
            printf("Time used Linear Search: %lf\n",((double)(endL)-startL)/CLOCKS_PER_SEC);
            break;
        }
        default : break;
    }

    free(input);
    free_collection(&collection);
    free_index(&index);
    clean_linear_search_data(&collection,doc_data,&inp_data);
    return 0;
}

char* take_user_input(){
    int capacity = 16;
    int length = 0;

    char *buffer = malloc(capacity * sizeof(char)); // free memory when finish use

    if(!buffer){
        return NULL;
    }

    int ch;

    while((ch = getchar()) != '\n' && ch != EOF){
        if(length + 1 >= capacity){
            capacity *= 2;
            char *newBuffer = realloc(buffer, capacity * sizeof(char));

            if(!newBuffer){
                free(buffer);
                return NULL;
            }
            
            buffer = newBuffer;
        }
        buffer[length++] = ch;
    }

    buffer[length] = '\0';

    return buffer;
}

void bubble_sort(SearchDocData data[], size_t count){
    for(int i=0; i < count ; i++){
        for(int j= i + 1; j < count; j++){
            if(data[i].score < data[j].score){
                SearchDocData temp;
                temp = data[i];
                data[i] = data[j];
                data[j] = temp;
            }
        }
    }
}
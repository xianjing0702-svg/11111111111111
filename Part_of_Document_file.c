#include <stdio.h>
#include <stdlib.h>

int main(void){
        FILE*fp=NULL;
        fp=fopen("20_newsgroups.txt","w+");

        if(fp == NULL){
        printf("cannot open this file!\n");
        return 1;
    }
        fprintf(fp,"This is the file we need\n");
        fputs("This is the file we need\n",fp);
        fclose(fp);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>  

int main(void)
{
    int *p = malloc(sizeof(int));
    if(p == NULL)
    {
        printf("Memory allocation failed.\n");
        return 1;   
    }

    *p = 100;   
    printf ("%d",*p);
    free(p);    
    p = NULL;   
    return 0;
}
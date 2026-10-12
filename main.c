
#include <stdio.h>
#include "document.h"
#include "tokenizer.h"

int main(void)
{
    Collection collection;

    /* 1. Initialise the collection */
    init_collection(&collection);

    /* 2. Load documents */
    int loaded = load_collection(
        &collection,
        "manifest.txt",
        "documents"
    );

    if (loaded <= 0)
    {
        printf("Failed to load documents.\n");
        free_collection(&collection);
        return 1;
    }

    printf("Loaded %d documents.\n", loaded);

    /* 3. Process all documents */
    if (!process_collection(&collection))
    {
        printf("Text processing failed.\n");
        free_collection(&collection);
        return 1;
    }

    /* 4. Print statistics */
    print_collection_statistics(&collection);

    /* 5. Print the first document */
    print_document(
        &collection,
        collection.documents[0].id
    );

    /* 6. Free memory */
    free_collection(&collection);

    return 0;
}
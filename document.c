
#include "document.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Initialise an empty collection */
void init_collection(Collection *collection)
{
    if (collection == NULL)
    {
        return;
    }

    collection->documents = NULL;
    collection->count = 0;
    collection->capacity = 0;
}

/* Free all memory used by the collection */
void free_collection(Collection *collection)
{
    size_t i;

    if (collection == NULL)
    {
        return;
    }

    for (i = 0; i < collection->count; i++)
    {
        free(collection->documents[i].text);
        collection->documents[i].text = NULL;
    }

    free(collection->documents);

    collection->documents = NULL;
    collection->count = 0;
    collection->capacity = 0;
}

/* Add capacity when more documents are needed */
static int reserve_documents(Collection *collection)
{
    size_t new_capacity;
    Document *new_documents;

    if (collection->count < collection->capacity)
    {
        return 1;
    }

    new_capacity = (collection->capacity == 0)
                       ? 16
                       : collection->capacity * 2;

    if (new_capacity < collection->capacity ||
        new_capacity > (size_t)-1 / sizeof(Document))
    {
        return 0;
    }

    new_documents = realloc(
        collection->documents,
        new_capacity * sizeof(Document)
    );

    if (new_documents == NULL)
    {
        return 0;
    }

    collection->documents = new_documents;
    collection->capacity = new_capacity;

    return 1;
}

/* Read an entire text file into memory */
static char *read_text_file(
    const char *path,
    size_t *text_length
)
{
    FILE *file;
    char *buffer;
    size_t length = 0;
    size_t capacity = 4096;
    size_t bytes_read;

    file = fopen(path, "rb");

    if (file == NULL)
    {
        return NULL;
    }

    buffer = malloc(capacity);

    if (buffer == NULL)
    {
        fclose(file);
        return NULL;
    }

    while (1)
    {
        if (capacity - length <= 1)
        {
            size_t new_capacity;
            char *new_buffer;

            if (capacity > (size_t)-1 / 2)
            {
                free(buffer);
                fclose(file);
                return NULL;
            }

            new_capacity = capacity * 2;

            new_buffer = realloc(buffer, new_capacity);

            if (new_buffer == NULL)
            {
                free(buffer);
                fclose(file);
                return NULL;
            }

            buffer = new_buffer;
            capacity = new_capacity;
        }

        bytes_read = fread(
            buffer + length,
            1,
            capacity - length - 1,
            file
        );

        length += bytes_read;

        if (bytes_read == 0)
        {
            if (ferror(file))
            {
                free(buffer);
                fclose(file);
                return NULL;
            }

            break;
        }
    }

    buffer[length] = '\0';

    fclose(file);

    *text_length = length;

    return buffer;
}

/* Check whether a filename is safe to use */
static int valid_filename(const char *filename)
{
    if (filename == NULL || filename[0] == '\0')
    {
        return 0;
    }

    if (strlen(filename) > MAX_FILENAME)
    {
        return 0;
    }

    if (strchr(filename, '/') != NULL ||
        strchr(filename, '\\') != NULL ||
        strcmp(filename, ".") == 0 ||
        strcmp(filename, "..") == 0)
    {
        return 0;
    }

    return 1;
}

/* Build the full path to a document */
static char *build_path(
    const char *folder,
    const char *filename
)
{
    size_t folder_length;
    size_t filename_length;
    int needs_separator;
    char *path;

    folder_length = strlen(folder);
    filename_length = strlen(filename);

    needs_separator =
        folder_length > 0 &&
        folder[folder_length - 1] != '/' &&
        folder[folder_length - 1] != '\\';

    if (folder_length > (size_t)-1 - filename_length - 2)
    {
        return NULL;
    }

    path = malloc(
        folder_length + filename_length +
        (needs_separator ? 2 : 1)
    );

    if (path == NULL)
    {
        return NULL;
    }

    strcpy(path, folder);

    if (needs_separator)
    {
        strcat(path, "/");
    }

    strcat(path, filename);

    return path;
}

/* Load documents listed in a manifest file.
   Supported formats:
   123 document.txt
   document.txt
*/
int load_collection(
    Collection *collection,
    const char *manifest_file,
    const char *documents_folder
)
{
    FILE *manifest;
    char line[1024];
    int loaded = 0;
    int next_id = 1;

    if (collection == NULL ||
        manifest_file == NULL ||
        documents_folder == NULL)
    {
        return 0;
    }

    manifest = fopen(manifest_file, "r");

    if (manifest == NULL)
    {
        perror("Could not open manifest file");
        return 0;
    }

    while (fgets(line, sizeof(line), manifest) != NULL)
    {
        char *start = line;
        char *end;
        char *filename;
        char *path;
        char *text;
        size_t text_length;
        int document_id = next_id;
        int parsed_id;
        char extra;

        /* Skip leading whitespace */
        while (isspace((unsigned char)*start))
        {
            start++;
        }

        /* Ignore empty lines and comments */
        if (*start == '\0' || *start == '#' ||
            *start == '\n' || *start == '\r')
        {
            continue;
        }

        /* Remove trailing whitespace */
        end = start + strlen(start);

        while (end > start &&
               isspace((unsigned char)end[-1]))
        {
            end--;
        }

        *end = '\0';

        /*
         * Accept either:
         * ID filename.txt
         * filename.txt
         */
        if (sscanf(start, "%d %254s %c",
                   &parsed_id, line, &extra) == 2)
        {
            document_id = parsed_id;
            filename = line;
        }
        else
        {
            filename = start;
        }

        if (!valid_filename(filename))
        {
            fprintf(stderr, "Invalid filename: %s\n", filename);
            continue;
        }

        path = build_path(documents_folder, filename);

        if (path == NULL)
        {
            fprintf(stderr, "Memory allocation failed.\n");
            break;
        }

        text = read_text_file(path, &text_length);
        free(path);

        if (text == NULL)
        {
            fprintf(stderr, "Could not read document: %s\n",
                    filename);
            continue;
        }

        if (!reserve_documents(collection))
        {
            free(text);
            fprintf(stderr, "Could not allocate document storage.\n");
            break;
        }

        Document *doc =
            &collection->documents[collection->count];

        doc->id = document_id;
        strcpy(doc->filename, filename);
        doc->text = text;
        doc->text_length = text_length;

        /* Calculated later by the text-processing module */
        doc->total_terms = 0;
        doc->unique_terms = 0;

        collection->count++;
        loaded++;

        next_id = document_id + 1;
    }

    if (ferror(manifest))
    {
        fprintf(stderr, "Error reading manifest file.\n");
    }

    fclose(manifest);

    return loaded;
}

/* Find a document by its ID */
const Document *get_document(
    const Collection *collection,
    int document_id
)
{
    size_t i;

    if (collection == NULL)
    {
        return NULL;
    }

    for (i = 0; i < collection->count; i++)
    {
        if (collection->documents[i].id == document_id)
        {
            return &collection->documents[i];
        }
    }

    return NULL;
}

/* Print one document */
void print_document(
    const Collection *collection,
    int document_id
)
{
    const Document *doc =
        get_document(collection, document_id);

    if (doc == NULL)
    {
        printf("Document ID %d not found.\n", document_id);
        return;
    }

    printf("\nDocument ID: %d\n", doc->id);
    printf("Filename: %s\n", doc->filename);
    printf("Text length: %zu bytes\n", doc->text_length);
    printf("Total terms: %ld\n", doc->total_terms);
    printf("Unique terms: %ld\n", doc->unique_terms);
    printf("\nContent:\n%s\n", doc->text);
}

/* Print collection statistics */
void print_collection_statistics(
    const Collection *collection
)
{
    size_t i;
    size_t total_bytes = 0;
    long total_terms = 0;
    long total_unique_terms = 0;

    if (collection == NULL)
    {
        return;
    }

    for (i = 0; i < collection->count; i++)
    {
        total_bytes += collection->documents[i].text_length;
        total_terms += collection->documents[i].total_terms;
        total_unique_terms += collection->documents[i].unique_terms;
    }

    printf("\n===== Collection Statistics =====\n");
    printf("Number of documents: %zu\n", collection->count);
    printf("Total text size: %zu bytes\n", total_bytes);
    printf("Total terms: %ld\n", total_terms);
    printf("Sum of unique terms per document: %ld\n",
           total_unique_terms);
}

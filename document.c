
#include "document.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * init_collection - PUBLIC API
 * ----------------------------
 * Input:
 *   collection - caller-owned Collection pointer; NULL is accepted safely.
 *
 * Action:
 *   Sets documents to NULL and count/capacity to 0. No files are loaded and
 *   no memory is allocated.
 *
 * Expected output:
 *   None. The Collection is ready to be passed to load_collection().
 *
 * Memory:
 *   Nothing to free here. Call free_collection() after loading/using documents.
 */
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

/*
 * free_collection - PUBLIC API
 * ----------------------------
 * Input:
 *   collection - Collection previously initialised/loaded; NULL is safe.
 *
 * Action:
 *   Frees each document's text buffer, then frees the dynamic documents array,
 *   and resets count/capacity to 0.
 *
 * Expected output:
 *   None. All pointers returned by get_document() become invalid.
 *
 * Memory:
 *   This is the required cleanup function for load_collection().
 */
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

/*
 * reserve_documents - INTERNAL HELPER (static)
 * --------------------------------------------
 * Input:
 *   collection - non-NULL Collection whose capacity may need to grow.
 *
 * Action:
 *   Ensures at least one free Document slot exists, using realloc() to double
 *   capacity when full.
 *
 * Expected output:
 *   Returns 1 on success or 0 on overflow/allocation failure.
 *
 * Memory:
 *   May reallocate collection->documents. The Collection owns the new block;
 *   free_collection() releases it later. Callers must not free it directly.
 */
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

/*
 * read_text_file - INTERNAL HELPER (static)
 * -----------------------------------------
 * Input:
 *   path        - filesystem path to an existing text file.
 *   text_length - output pointer receiving the number of bytes read.
 *
 * Action:
 *   Opens the file in binary mode and reads the entire contents into a growing
 *   heap buffer, adding a terminating '\0'.
 *
 * Expected output:
 *   Returns a malloc-allocated, null-terminated string on success; NULL on
 *   open/read/allocation failure.
 *
 * Memory:
 *   The returned buffer is owned by the caller and must be freed. On success,
 *   load_collection() stores it as Document.text.
 */
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

/*
 * valid_filename - INTERNAL HELPER (static)
 * -----------------------------------------
 * Input:
 *   filename - relative path read from the manifest.
 *
 * Action:
 *   Rejects NULL/empty names, names longer than MAX_FILENAME, absolute paths,
 *   Windows drive-letter paths, and empty/"."/".." path components. Safe
 *   relative subpaths such as comp.graphics/37261 are accepted.
 *
 * Expected output:
 *   Returns 1 when safe to combine with the documents folder, else 0.
 *
 * Memory:
 *   No allocation and nothing to free.
 */
static int valid_filename(const char *filename)
{
    const char *component;

    if (filename == NULL || filename[0] == '\0')
    {
        return 0;
    }

    if (strlen(filename) > MAX_FILENAME)
    {
        return 0;
    }

    /* Reject absolute paths and Windows drive-letter paths. */
    if (filename[0] == '/' || filename[0] == '\\' ||
        (isalpha((unsigned char)filename[0]) && filename[1] == ':'))
    {
        return 0;
    }

    /* Reject empty, "." and ".." path components. */
    component = filename;

    while (*component != '\0')
    {
        const char *start = component;
        size_t length;

        while (*component != '\0' &&
               *component != '/' &&
               *component != '\\')
        {
            component++;
        }

        length = (size_t)(component - start);

        if (length == 0 ||
            (length == 1 && start[0] == '.') ||
            (length == 2 && start[0] == '.' && start[1] == '.'))
        {
            return 0;
        }

        if (*component != '\0')
        {
            component++;
        }
    }

    return 1;
}

/*
 * build_path - INTERNAL HELPER (static)
 * -------------------------------------
 * Input:
 *   folder   - base directory containing documents.
 *   filename - validated relative filename/path.
 *
 * Action:
 *   Allocates and joins folder and filename, inserting '/' when needed.
 *
 * Expected output:
 *   Returns a malloc-allocated path string, or NULL on allocation failure.
 *
 * Memory:
 *   The caller owns the returned path and must free() it after fopen/read.
 */
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

/*
 * load_collection - PUBLIC API
 * ----------------------------
 * Input:
 *   collection       - initialised Collection that receives loaded documents.
 *   manifest_file    - path to a manifest containing one filename per line.
 *                      Lines may be "filename.txt" or "id filename.txt".
 *                      Blank lines and lines beginning with '#' are skipped.
 *   documents_folder - base folder prepended to each manifest filename.
 *
 * Action:
 *   Opens the manifest, validates each relative filename, reads each document
 *   completely into memory, and appends a populated Document to the Collection.
 *
 * Expected output:
 *   Returns the number of documents successfully loaded, or 0 when arguments
 *   are invalid/the manifest cannot be opened. Error details go to stderr.
 *   Each loaded Document gets an ID, filename, text, text_length, and zeroed
 *   term statistics (later filled by process_document()).
 *
 * Memory:
 *   Allocates collection->documents and each Document.text. The caller MUST
 *   call free_collection() when finished. Do not free individual document text.
 *
 * Example:
 *   Collection c;
 *   init_collection(&c);
 *   int n = load_collection(&c, "manifest.txt", "documents");
 *   if (n > 0) { printf("Loaded %d documents\n", n); }
 *   free_collection(&c);
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

/*
 * get_document - PUBLIC API
 * -------------------------
 * Input:
 *   collection  - Collection to search; NULL returns NULL.
 *   document_id - ID to locate.
 *
 * Action:
 *   Performs a linear search through collection->documents[].
 *
 * Expected output:
 *   Returns a borrowed pointer to the matching Document, or NULL if absent.
 *
 * Memory:
 *   Do NOT free the returned pointer. It is owned by the Collection and remains
 *   valid only until free_collection() is called.
 */
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

/*
 * print_document - PUBLIC API
 * ---------------------------
 * Input:
 *   collection  - Collection containing the document; may be NULL.
 *   document_id - ID of the document to display.
 *
 * Action:
 *   Looks up the document and prints metadata, term statistics, and full text.
 *
 * Expected output:
 *   Text on stdout, or "Document ID <id> not found." when missing.
 *
 * Memory:
 *   No allocation and nothing for the caller to free.
 */
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

/*
 * print_collection_statistics - PUBLIC API
 * ----------------------------------------
 * Input:
 *   collection - Collection to summarise; may be NULL.
 *
 * Action:
 *   Sums text bytes, total terms, and unique terms across all documents.
 *
 * Expected output:
 *   Prints a statistics block to stdout. Term totals are meaningful only after
 *   process_collection() has filled the per-document term fields.
 *
 * Memory:
 *   No allocation and nothing for the caller to free.
 */
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

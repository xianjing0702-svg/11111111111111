/*
 * document.h - Public API for document and collection management.
 *
 * PURPOSE
 *   Loads text files into memory and exposes them as Document objects inside
 *   a Collection. Other modules should use this header rather than opening
 *   document files directly.
 *
 * TYPICAL USE
 *   Collection collection;
 *   init_collection(&collection);
 *
 *   int loaded = load_collection(&collection, "manifest.txt", "documents");
 *   if (loaded > 0)
 *   {
 *       const Document *doc = get_document(&collection, collection.documents[0].id);
 *       if (doc != NULL)
 *       {
 *           printf("Loaded: %s\n", doc->filename);
 *       }
 *   }
 *
 *   free_collection(&collection);   // REQUIRED when finished
 */

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <stddef.h>

/* Maximum number of characters in a stored relative filename/path. */
#define MAX_FILENAME 255

/*
 * Document - One loaded text file and its metadata.
 *
 * MEMORY OWNERSHIP
 *   The Collection that owns this Document also owns 'text'. Do not free
 *   document->text yourself while the Document is still in the Collection.
 */
typedef struct
{
    int id;                       /* Unique document identifier. */
    char filename[MAX_FILENAME + 1]; /* Relative filename/path used to load it. */
    char *text;                   /* Entire file contents, null-terminated. */
    size_t text_length;           /* Number of bytes in text, excluding '\0'. */
    long total_terms;             /* Filled later by process_document(). */
    long unique_terms;            /* Filled later by process_document(). */
} Document;

/*
 * Collection - Dynamically allocated array of Document objects.
 *
 * MEMORY OWNERSHIP
 *   collection->documents and every document->text are allocated by
 *   load_collection(). The caller must call free_collection() when done.
 */
typedef struct
{
    Document *documents;          /* Array of loaded documents. */
    size_t count;                 /* Number of loaded documents. */
    size_t capacity;              /* Allocated array capacity. */
} Collection;

/*
 * init_collection
 * ----------------
 * WHAT IT DOES
 *   Initialises a Collection to an empty, safe state. It does not allocate
 *   memory and does not load files.
 *
 * PARAMETERS
 *   collection : Pointer to a Collection owned by the caller. May be NULL.
 *
 * RETURNS
 *   Nothing.
 *
 * MEMORY
 *   No allocation is performed. This must be called before load_collection().
 *   The collection must later be released with free_collection().
 *
 * EXAMPLE
 *   Collection c;
 *   init_collection(&c);
 */
void init_collection(Collection *collection);

/*
 * free_collection
 * ---------------
 * WHAT IT DOES
 *   Frees every Document.text allocated by load_collection(), then frees the
 *   dynamic documents array, and resets the Collection to empty.
 *
 * PARAMETERS
 *   collection : Collection to release. May be NULL.
 *
 * RETURNS
 *   Nothing.
 *
 * MEMORY
 *   REQUIRED after successful loading. After this call no document pointers
 *   obtained from get_document() remain valid.
 *
 * EXAMPLE
 *   free_collection(&c);
 */
void free_collection(Collection *collection);

/*
 * load_collection
 * ---------------
 * WHAT IT DOES
 *   Reads a manifest file and loads each listed document from documents_folder.
 *   Each loaded file is stored as a null-terminated string in Document.text.
 *   It appends to any documents already present in the collection.
 *
 * PARAMETERS
 *   collection       : Initialised Collection that receives the documents.
 *   manifest_file    : Path to a text manifest. Blank lines and lines starting
 *                      with '#' are ignored. Each remaining line may be either:
 *                          filename.txt
 *                      or:
 *                          document_id filename.txt
 *                      Relative subpaths such as comp.graphics/37261 are allowed.
 *   documents_folder : Folder prepended to each manifest filename.
 *
 * RETURNS
 *   Number of successfully loaded documents. Returns 0 on an invalid argument,
 *   unreadable manifest, or when no documents could be loaded.
 *
 * OUTPUT
 *   Writes error messages to stderr for invalid filenames, unreadable files,
 *   and allocation failures. Successfully loaded entries are added to
 *   collection->documents[0..collection->count-1].
 *
 * MEMORY
 *   Allocates collection->documents and each document->text. The caller must
 *   call free_collection() when finished. Individual document text must not be
 *   freed directly by callers.
 *
 * EXAMPLE
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
);

/*
 * get_document
 * ------------
 * WHAT IT DOES
 *   Searches the Collection linearly by document ID.
 *
 * PARAMETERS
 *   collection  : Collection to search. May be NULL.
 *   document_id : ID to find.
 *
 * RETURNS
 *   Pointer to the matching Document, or NULL if not found or collection is NULL.
 *
 * MEMORY
 *   Returns a borrowed pointer owned by the Collection. Do NOT free it. It is
 *   valid only while the Collection is alive and until free_collection() is
 *   called.
 *
 * EXAMPLE
 *   const Document *doc = get_document(&c, 1);
 *   if (doc != NULL) { printf("%s\n", doc->text); }
 */
const Document *get_document(
    const Collection *collection,
    int document_id
);

/*
 * print_document
 * --------------
 * WHAT IT DOES
 *   Prints one document's ID, filename, size, term statistics, and full text
 *   to stdout.
 *
 * PARAMETERS
 *   collection  : Collection containing the document. May be NULL.
 *   document_id : ID of the document to print.
 *
 * RETURNS
 *   Nothing.
 *
 * OUTPUT
 *   Prints the document details, or "Document ID <id> not found." when absent.
 *
 * MEMORY
 *   No allocation and nothing to free.
 */
void print_document(
    const Collection *collection,
    int document_id
);

/*
 * print_collection_statistics
 * ---------------------------
 * WHAT IT DOES
 *   Sums and prints collection-wide statistics: document count, total text
 *   bytes, total terms, and the sum of unique terms per document.
 *
 * PARAMETERS
 *   collection : Collection to report on. May be NULL.
 *
 * RETURNS
 *   Nothing.
 *
 * OUTPUT
 *   Prints a statistics block to stdout. Term values are meaningful only after
 *   process_collection() or process_document() has been called.
 *
 * MEMORY
 *   No allocation and nothing to free.
 */
void print_collection_statistics(
    const Collection *collection
);

#endif

#ifndef SIMPLE_MALLOC_LIBRARY_INCLUDED
#define SIMPLE_MALLOC_LIBRARY_INCLUDED

#include <stddef.h>

// returns a pointer to a region of memory at least 'n' bytes long,
// or NULL if memory could not be allocated. returns NULL if 0 bytes are
// requested. memory allocated this way should be freed when not needed anymore
// by calling simple_free() on the pointer returned by simple_malloc().
void* simple_malloc(size_t n);

// frees a chunk of memory allocated by simple_malloc(). behavior is undefined
// if called on a pointer that was not allocated by simple_malloc() (but it will
// be bad), unless the pointer is NULL, in which case it will not do anything.
void  simple_free(void* ptr);

#endif // SIMPLE_MALLOC_LIBRARY_INCLUDED

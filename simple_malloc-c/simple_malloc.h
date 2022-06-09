/*
 * MIT License
 *
 * Copyright 2022 Jenna Fligor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

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

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

#include <stddef.h>
#include <stdint.h>

#include <stdio.h>

#include <unistd.h>
#include <sys/mman.h>

#define MIN_SIZE sizeof(FreeNode)

typedef struct {
    size_t size;
    void* prev;
    void* next;
} FreeNode;

size_t g_pagesize = 0;

FreeNode* g_list_front = NULL;
FreeNode* g_list_back  = NULL;

void print_node(FreeNode* node) {
    printf("node: %p\n"
            "    size: %lu\n"
            "    prev: %p\n"
            "    next: %p\n",
            node,
            node->size,
            node->prev,
            node->next);
}

void print_tree() {
    FreeNode* node = g_list_front;
    for (;;) {
        print_node(node);
        if (node->next == NULL) break;
        node = node->next;
    }
}

void append_node(FreeNode* node) {
    if (g_list_front == NULL && g_list_back == NULL) {
        node->next = NULL;
        node->prev = NULL;
        g_list_front = node;
        g_list_back = node;
        return;
    }

    node->prev = g_list_back;
    g_list_back->next = node;

    node->next = NULL;
    g_list_back = node;
}

int new_pages(size_t size) {
    if (g_pagesize == 0) g_pagesize = sysconf(_SC_PAGESIZE);

    size_t size_page_aligned = ((size + (g_pagesize - 1)) / g_pagesize) * g_pagesize;

    printf("new pages size: %lu\n", size_page_aligned);

    void* pages = mmap(NULL, size_page_aligned, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (pages == MAP_FAILED) return 1;

    ((FreeNode*)pages)->size = size_page_aligned;
    append_node(pages);

    return 0;
}

void* simple_malloc(size_t n) {
    if (n == 0) return NULL;

    size_t size = n;
    if (size < MIN_SIZE) size = MIN_SIZE;
    else size = ((size + (8 - 1)) / 8) * 8;
    size += sizeof(size_t);

    printf("allocation size: %lu\n", size);

    if (g_list_front == NULL) {
        printf("free list empty, getting new pages\n");
        if (new_pages(size) != 0) return NULL;
    }

    FreeNode* node = g_list_front;
    for (;;) {
        if (node->size >= size) break;
        if (node->next != NULL) {
            node = node->next;
        } else {
            if (new_pages(size) != 0) return NULL;
            node = node->next;
        }
    }

    if (node->size >= size + MIN_SIZE) {
        FreeNode* new_node = (void*)((size_t)node+size);
        new_node->size = node->size - size;
        node->size = size;
        append_node(new_node);
        printf("trimmed node size\n");
    }

    FreeNode* prev_node = node->prev;
    FreeNode* next_node = node->next;

    if (next_node == NULL && prev_node == NULL) {
        g_list_front = NULL;
        g_list_back = NULL;
    } else if (prev_node == NULL) {
        next_node->prev = NULL;
        g_list_front = next_node;
    } else if (next_node == NULL) {
        prev_node->next = NULL;
        g_list_back = prev_node;
    } else {
        prev_node->next = next_node;
        next_node->prev = prev_node;
    }

    return (void*)((size_t)node+sizeof(size_t));
}

void simple_free(void* ptr) {
    printf("ptr: %p\n", ptr);
    FreeNode* node = (FreeNode*)((size_t)ptr-sizeof(size_t));
    node->prev = NULL;
    node->next = NULL;
    printf("node: %p\n", node);

    if (g_list_front == NULL) {
        g_list_front = node;
        return;
    }

    {
        FreeNode* list_node = g_list_front;
        for (;;) {
            if (((size_t)list_node)+((size_t)list_node->size) == (size_t)node) {
                printf("found contiguous block to left\n");
                list_node->size += node->size;
                node = list_node;
                break;
            }
            if (list_node->next == NULL) break;
            list_node = list_node->next;
        }
    }


    {
        size_t end_addr = ((size_t)node)+((size_t)node->size);
        FreeNode* list_node = g_list_front;
        for (;;) {
            if (end_addr == (size_t)list_node) {
                printf("found contiguous block to right\n");
                node->size += list_node->size;

                FreeNode* prev_node = list_node->prev;
                FreeNode* next_node = list_node->next;
                if (prev_node == NULL && next_node == NULL) {
                    g_list_front = node;
                    g_list_back = node;
                    break;
                } else if (prev_node == NULL) {
                    next_node->prev = NULL;
                    g_list_front = next_node;
                } else if (next_node == NULL) {
                    prev_node->next = NULL;
                    g_list_back = prev_node;
                } else {
                    prev_node->next = next_node;
                    next_node->prev = prev_node;
                }
                append_node(node);
                return;
            }
            if (list_node->next == NULL) break;
            list_node = list_node->next;
        }
    }

    append_node(node);
    return;
}



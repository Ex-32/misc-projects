
#include "try.h"
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct Node {
    struct Node* next;
    jmp_buf val;
    bool used;
};

struct Node* stack_top = NULL;

void sigsegv_handler(int unused) {
    (void)unused;
    if (stack_top == NULL || stack_top->used) {
        signal(SIGSEGV, SIG_DFL);
        return;
    }
    stack_top->used = true;
    siglongjmp(stack_top->val, 1);
}

void try_catch_init() { signal(SIGSEGV, sigsegv_handler); }

void INTERNAL_push_jmpbuf_stack() {
    struct Node* new_node = calloc(1, sizeof(struct Node));
    new_node->next = stack_top;
    new_node->used = false;
    stack_top = new_node;
}
void INTERNAL_pop_jmpbuf_stack() {
    if (stack_top == NULL) {
        fprintf(
            stderr,
            "Error: attempted to pop jmp_buf off empty TRY/CATCH stack\n"
            "(likley unpaired TRY/END_TRY macros)\n"
        );
        exit(EXIT_FAILURE);
    }

    struct Node* under = stack_top->next;
    free(stack_top);
    stack_top = under;
}
jmp_buf* INTERNAL_peak_jmpbuf_stack() { return &stack_top->val; }

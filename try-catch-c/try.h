#pragma once
#include <setjmp.h>

void try_catch_init();

void INTERNAL_push_jmpbuf_stack();
void INTERNAL_pop_jmpbuf_stack();
jmp_buf* INTERNAL_peak_jmpbuf_stack();

#define TRY                                                                    \
    INTERNAL_push_jmpbuf_stack();                                              \
    if (sigsetjmp(*INTERNAL_peak_jmpbuf_stack(), 1) == 0)

#define CATCH else
#define END_TRY INTERNAL_pop_jmpbuf_stack();

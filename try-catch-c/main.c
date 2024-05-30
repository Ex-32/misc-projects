#include "try.h"
#include <stdio.h>

#define DIE (void)*(volatile int*)0
int main() {
    try_catch_init();

    TRY {
        printf("about to sefault\n");
        DIE;
    }
    CATCH { printf("we segfaulted, anyway...\n"); }
    END_TRY
    printf("end of TRY block\n");

    printf("about to sefault\n");
    DIE;
}

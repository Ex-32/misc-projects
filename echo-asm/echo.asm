; MIT License
;
; Copyright (c) 2022 Jenna Fligor
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.

; compile: nasm -felf64 echo.asm
; link: ld -m elf_x86_64 -o echo echo.o
; usage: echo [-n] [STRING]...
; (adding the '-n' flag or ending the final string with '\c' will cause it to
; not add a newline before returning)

global _start

SECTION .text
CHAR_LF:    db 10
CHAR_SPACE: db 32

_start:
    mov     r15, 1     ; flag for printing a line feed

    mov     r13, [rsp] ; r13 holds a int of the number of command line args

    mov     r14, rsp   ; r14 holds a char** to the 1st command line argument
    add     r14, 16    ; (not the 0th [the name of the command is excluded])

    cmp     r13, 1     ; if there was only one argument (the name of the
    jz      print_lf   ; command) we can skip all the printing code except the
    dec     r13        ; newline at the end

    ; here we iterate over the first command line arguemnt to see if it's
    ; exactly '-n' if it is, then we set the lf flag (r15) to 0
    ; and skip over this argument for printing
    mov     rax, [r14]      ; rax is a char* to the first arguemnt

    cmp     byte [rax], 45  ; 45 = '-'
    jnz     print_setup
    inc     rax
    cmp     byte [rax], 110 ; 110 = 'n'
    jnz     print_setup
    inc     rax
    cmp     byte [rax], 0   ; 0 = null termnator
    jnz     print_setup

    mov     r15, 0          ; set lf flag to 0
    add     r14, 8          ; set r14 to the next argument
    dec     r13             ; decrease count of remaining arguemnts

print_setup:
    lea     r12, [CHAR_SPACE]
    mov     rdi, 1          ; set file discriptor to print to as stdout
    cmp     r13, 0          ; check to make sure there's at least 1 argument
    jz      print_break     ; remaining, else jump to end of print loop

print_loop:
    mov     rsi, [r14]
    mov     rdx, rsi

len_loop:
    cmp     byte [rdx], 0
    jz      escape_check
    inc     rdx
    jmp     len_loop

escape_check:
    cmp     r13, 1          ; unless this is the last arguemnt, we can skip
    jnz     print_arg       ; this step

    mov     rax, rdx        ; rdx is a char* to the null terminator of the arg
    sub     rax, 2          ; we're looking at the last two chars of the arg

    cmp     byte [rax], 92  ; 92 = '\'
    jnz     print_arg
    inc     rax
    cmp     byte [rax], 99  ; 99 = 'c'
    jnz     print_arg

    mov     r15, 0          ; set lf flag to 0
    sub     rdx, 2          ; remove those two chars from what's to be printed

print_arg:
    sub     rdx, rsi        ; get length of arg in rdx

    mov     rax, 1          ; set rax to sys_write
    syscall

    add     r14, 8          ; advance char** to next char*
    dec     r13             ; decrease remaining arguemnt count
    jz     print_break

    ; print space
    mov     rsi, r12
    mov     rdx, 1
    mov     rax, 1
    syscall

    ; rinse, repeat
    jmp     print_loop

print_break:
    ; if the line feed flag isn't set, skip printing the newline
    cmp     r15, 0
    jz      exit

print_lf:
    lea     rsi, [CHAR_LF]  ; newline char
    mov     rdx, 1          ; str length
    mov     rax, 1          ; sys_write
    syscall

exit:
    mov     rdi, 0      ; exit code
    mov     rax, 60     ; sys_exit
    syscall

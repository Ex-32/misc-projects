format ELF64

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

; assemble: `fasm allocator.asm`
; static library: `ar rcs allocator.a allocator.o`

; null pointer value
NULL = 0x0

; the standard page size on linux is 0x1000 (4096) bytes
; if this is not the case on your system, the library may break,
; or at least leave
page_size = 0x1000

; minmum allocation size; the size of the freelist node structure
; ( a 64-bit unsigned integer, and 2 pointers)
min_size = 24
; offsets for reading freelist node data
size_offset = 0
prev_offset = 8
next_offset = 16

; mmap argument values
sys_mmap = 9
PROT_READ_WRITE = 0x3 ; 0x1 | 0x2
MAP_PRIVATE_ANONYMOUS = 0x22 ; 0x02 | 0x20
MAP_FAILED = qword -1

; symbol exports
public mem_get
public mem_ret

SECTION '.data' writeable
freelist_front dq 0 ; pointer to start of free-memory list
freelist_back  dq 0 ; pointer to end of free-memory list
freelist_locked db 0 ; mutex for access to free-memory list (0 == unlocked)

SECTION '.text' executable
mem_get: ; PUBLIC
    cmp rdi, NULL
    jnz _mem_get_not_null
    mov rax, NULL
    ret

_mem_get_not_null:
    add rdi, 8
    cmp rdi, min_size
    jge _mem_get_size_checked

    mov rdi, min_size

_mem_get_size_checked:
    mov rax, 8 - 1
    add rdi, rax
    not rax
    and rdi, rax

    push rdi
    call get_lock
    pop rdi

    mov rax, qword [freelist_front]
    cmp rax, NULL
    jnz _mem_get_freelist_not_empty

    push rdi
    call new_pages
    pop rdi
    cmp rax, 0
    jnz _mem_get_failed

_mem_get_freelist_not_empty:
    mov rax, qword [freelist_front]

_mem_get_freelist_search_loop:
    cmp qword [rax+size_offset], rdi
    jge _mem_get_freelist_search_break
    cmp qword [rax+next_offset], NULL
    jnz _mem_get_freelist_search_next
    push rdi
    push rax
    call new_pages
    cmp rax, 0
    jnz _mem_get_failed
    pop rax
    pop rdi
    jmp _mem_get_freelist_not_empty

_mem_get_freelist_search_next:
    mov rax, qword [rax+next_offset]
    jmp _mem_get_freelist_search_loop

_mem_get_freelist_search_break:
    mov rsi, rdi
    add rsi, min_size
    cmp qword [rax+size_offset], rsi
    jl _mem_get_no_node_split

    mov rsi, rax
    add rsi, rdi

    mov rdx, qword [rax+size_offset]
    sub rdx, rdi
    mov qword [rsi+size_offset], rdx

    mov qword [rax+size_offset], rdi

    push rdi
    push rax
    mov rdi, rsi
    call append_node
    pop rax
    pop rdi

_mem_get_no_node_split:
    push rax
    mov rdi, rax
    call remove_node
    call ret_lock
    pop rax
    add rax, 8
    ret

_mem_get_failed:
    call ret_lock
    mov rax, NULL
    ret

mem_ret: ; PUBLIC
    cmp rdi, NULL
    jnz _mem_ret_not_null
    ret

_mem_ret_not_null:
    sub rdi, 8

    mov qword [rdi+prev_offset], NULL
    mov qword [rdi+next_offset], NULL

    push rdi
    call get_lock
    pop rdi

    cmp [freelist_front], NULL
    jnz _mem_ret_freelist_not_empty

    mov [freelist_front], rdi
    mov [freelist_back], rdi
    jmp _mem_ret_freelist_done

_mem_ret_freelist_not_empty:
    mov dh, 0 ; joined to left flag
    mov dl, 0 ; joined to right flag

    mov rsi, rdi
    add rsi, qword [rdi+size_offset]

    mov rax, qword [freelist_front]

_mem_ret_freelist_search_loop:
    cmp dh, byte 0
    jnz _mem_ret_freelist_search_loop_no_left

    mov rcx, rax
    add rcx, qword [rax+size_offset]
    cmp rcx, rdi
    jnz _mem_ret_freelist_search_loop_no_left

    mov rcx, qword [rax+size_offset]
    add rcx, qword [rdi+size_offset]
    mov qword [rax+size_offset], rcx

    mov rdi, rax
    mov dh, byte 1
    cmp dl, byte 0
    jnz _mem_ret_freelist_search_break

_mem_ret_freelist_search_loop_no_left:
    cmp dl, byte 0
    jnz _mem_ret_freelist_search_loop_no_right
    cmp rax, rsi
    jnz _mem_ret_freelist_search_loop_no_right

    mov rcx, qword [rax+size_offset]
    add rcx, qword [rdi+size_offset]
    mov qword [rdi+size_offset], rcx

    push rax
    push rdi
    push rdx
    mov rdi, rax
    call remove_node
    pop rdx
    pop rdi
    pop rax

    mov dl, byte 1
    cmp dh, byte 0
    jnz _mem_ret_freelist_search_break

_mem_ret_freelist_search_loop_no_right:
    cmp qword [rax+next_offset], NULL
    jz _mem_ret_freelist_search_break
    mov rax, qword [rax+next_offset]
    jmp _mem_ret_freelist_search_loop

_mem_ret_freelist_search_break:
    cmp dh, byte 0
    jnz _mem_ret_freelist_done
    call append_node

_mem_ret_freelist_done:
    call ret_lock
    ret

append_node:
    cmp qword [freelist_front], NULL
    jnz _append_node_freelist_not_empty

    mov qword [rdi+prev_offset], NULL
    mov qword [rdi+next_offset], NULL

    mov qword [freelist_front], rdi
    mov qword [freelist_back], rdi
    ret

_append_node_freelist_not_empty:

    ; make new node->prev point to old freelist_back
    mov rax, qword [freelist_back]
    mov qword [rdi+prev_offset], rax

    ; make old freelist_back->next point to new node
    mov qword [rax+next_offset], rdi

    ; make new node->next NULL
    mov qword [rdi+next_offset], NULL

    ; make new node the freelist_back
    mov qword [freelist_back], rdi

    ret

remove_node:
    cmp rdi, NULL
    jnz _remove_node_not_null
    ret

_remove_node_not_null:

    mov r8, qword [rdi+prev_offset] ; pointer to prev node
    mov r9, qword [rdi+next_offset] ; pointer to next node

    cmp r8, NULL
    jz _remove_node_prev_null
    cmp r9, NULL
    jz _remove_node_next_null

    ; neither is null
    mov qword [r8+next_offset], r9
    mov qword [r9+prev_offset], r8
    ret

_remove_node_prev_null:
    cmp r9, NULL
    jz _remove_node_both_null

    mov qword [r9+prev_offset], NULL
    mov qword [freelist_front], r9
    ret

_remove_node_next_null:
    mov qword [r8+next_offset], NULL
    mov qword [freelist_back], r8
    ret

_remove_node_both_null:
    mov qword [freelist_back], NULL
    mov qword [freelist_front], NULL
    ret

new_pages:
    mov rax, page_size-1
    add rdi, rax
    not rax
    and rdi, rax
    push rdi

    mov rsi, rdi
    mov rdi, NULL
    mov rdx, PROT_READ_WRITE
    mov r10, MAP_PRIVATE_ANONYMOUS
    mov r8, -1
    mov r9, NULL
    mov rax, sys_mmap
    syscall

    cmp rax, MAP_FAILED
    jz _new_pages_failed

    mov rdi, rax
    pop rax
    mov qword [rdi+size_offset], rax

    call append_node
    mov rax, 0
    ret

_new_pages_failed:
    mov rax, 1
    ret

get_lock:
    mov al, 1
    xchg byte [freelist_locked], al
    cmp al, 0
    jnz get_lock
    ret

ret_lock:
    mov al, 0
    xchg [freelist_locked], al
    ret

# memory_allocator (asm)

This is an implementation of a basic, first-fit memory allocator written in
FASM x86_64 assembly for linux using just kernel system calls.

The allocator was designed to conform to the SysV ABI; it defines two public
symbols `mem_get` which accepts one argument `n` (a 64-bit unsigned integer) and
returns a pointer to a block of memory of at least `n` bytes; and `mem_ret`
which accepts one argument `p` (a pointer to a block of memory returned by
`mem_get`) and returns nothing, this returns the memory to the allocator for
future allocations. calling `mem_ret` on a pointer not returned by `mem_get`
is undefined (but bad).

the following C forward declarations _should_ be accurate (on 64-bit platforms
using the SysV ABI):

```c
void* mem_get(size_t n);
void  mem_ret(void*  p);
```

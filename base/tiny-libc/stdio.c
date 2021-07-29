#include <stdio.h>
#include <stdint.h>
#include <string.h>

void putc(char c) {
    char smol[2];
    smol[0] = c;
    smol[1] = '\0';
    puts(smol);
}

void puts(char* s) {
    int len = strlen(s);
    asm volatile("      \
        mov %0, %%rax;  \
        mov %1, %%rbx;  \
        mov %2, %%rcx;  \
        int $0x80;      \
    " : : "r" ((uint64_t)1), "r" ((uint64_t)s), "r" ((uint64_t)len) : "rax", "rbx", "rcx", "memory");
}
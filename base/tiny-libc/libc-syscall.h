#ifndef LIBC_SYSCALL
#define LIBC_SYSCALL


#define SYSCALL_DEBUGWRITE 1
#define SYSCALL_MMAP 2
#define SYSCALL_MUNMAP 3
#define SYSCALL_EXIT 4
#define SYSCALL_FORK 5
#define SYSCALL_IPCHINT 6
#define SYSCALL_IPCSENDPID 7
#define SYSCALL_IPCSENDPIPE 8
#define SYSCALL_IPCRECV 9

#include <stdint.h>


__attribute__((always_inline)) static inline void syscall_2arg_0ret(uint64_t syscall, uint64_t arg1, uint64_t arg2) {
    asm volatile("      \
        mov %0, %%rax;  \
        mov %1, %%rbx;  \
        mov %2, %%rcx;  \
        int $0x80;      \
    " : : "r" ((uint64_t)syscall), "r" ((uint64_t)arg1), "r" ((uint64_t)arg2) : "rax", "rbx", "rcx", "memory");
    asm volatile("": : :"memory");
}

__attribute__((always_inline)) static inline uint64_t syscall_0arg_1ret(uint64_t syscall) {
    uint64_t ret;
    asm volatile("      \
        mov %1, %%rax;  \
        int $0x80;      \
        mov %%rax, %0   \
    " : "=r" (ret) : "r" ((uint64_t)syscall) : "rax", "memory");
    asm volatile("": : :"memory");
    return ret;
}

__attribute__((always_inline)) static inline uint64_t syscall_3arg_1ret(uint64_t syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    uint64_t ret;
    asm volatile("      \
        mov %1, %%rax;  \
        mov %2, %%rbx;  \
        mov %3, %%rcx;  \
        mov %4, %%rdx;  \
        int $0x80;      \
        mov %%rax, %0   \
    " : "=r" (ret) : "r" ((uint64_t)syscall), "r" ((uint64_t)arg1), "r" ((uint64_t)arg2), "r" ((uint64_t)arg3) : "rax", "rbx", "rcx", "rdx", "memory");
    asm volatile("": : :"memory");
    return ret;
}

__attribute__((always_inline)) static inline uint64_t syscall_4arg_1ret(uint64_t syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    uint64_t ret;
    asm volatile("      \
        mov %1, %%rax;  \
        mov %2, %%rbx;  \
        mov %3, %%rcx;  \
        mov %4, %%rdx;  \
        mov %5, %%rdi;  \
        int $0x80;      \
        mov %%rax, %0   \
    " : "=r" (ret) : "r" ((uint64_t)syscall), "r" ((uint64_t)arg1), "r" ((uint64_t)arg2), "r" ((uint64_t)arg3), "r" ((uint64_t)arg4) : "rax", "rbx", "rcx", "rdx", "memory");
    asm volatile("": : :"memory");
    return ret;
}


#endif
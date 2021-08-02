#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include <libc-syscall.h>

int puts(const char* s) {
    size_t len = strlen(s);
    syscall_2arg_0ret(SYSCALL_DEBUGWRITE, (uint64_t)s, len);
    return 1; // non-negative value
}

int debug_printf(const char* str, ...) {
    va_list args;
    va_start(args, str);
    while(*str) {
        if(*str == '%') {
            switch(*++str) {
                case 0: puts("debug_printf: null terminator after %\r\n"); return -1;
                case '%': syscall_2arg_0ret(SYSCALL_DEBUGWRITE, (uint64_t)str, 1); break;
                case 's': puts(va_arg(args, const char*)); break;
                default: puts("debug_printf: invalid char after %\r\n"); return -1;
            }
        } else {
            syscall_2arg_0ret(SYSCALL_DEBUGWRITE, (uint64_t)str++, 1);
        }
    }
    va_end(args);
    return 0;
}
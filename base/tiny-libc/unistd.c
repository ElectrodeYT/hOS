#include <unistd.h>
#include <libc-syscall.h>

pid_t fork() {
    return (int)syscall_0arg_1ret(SYSCALL_FORK);
}
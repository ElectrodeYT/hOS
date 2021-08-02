#include <ipc.h>
#include <libc-syscall.h>
#include <string.h>
#include <errno.h>

int64_t ipchint(uint64_t msg_len, char* named_pipe) {
    uint64_t named_pipe_len = 0;
    if(named_pipe) {
        named_pipe_len = strlen(named_pipe);
    }
    return (int64_t)syscall_3arg_1ret(SYSCALL_IPCHINT, msg_len, (uint64_t)named_pipe, named_pipe_len);
}

int64_t ipcrecv(void* buffer, size_t buffer_size, int should_block) {
    return (int64_t)syscall_3arg_1ret(SYSCALL_IPCRECV, (uint64_t)buffer, buffer_size, should_block);
}

int64_t ipcsendpipe(const void* msg, size_t msg_size, char* pipe) {
    if(!pipe) { return -EINVAL; }
    return (int64_t)syscall_4arg_1ret(SYSCALL_IPCSENDPIPE, (uint64_t)msg, msg_size, (uint64_t)pipe, strlen(pipe));
}
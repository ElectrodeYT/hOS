#ifndef IPC_H
#define IPC_H
#include <stdint.h>
#include <stddef.h>

int64_t ipchint(uint64_t msg_len, char* named_pipe);
int64_t ipcrecv(void* buffer, size_t buffer_size, int should_block);
int64_t ipcsendpipe(const void* msg, size_t msg_size, char* pipe);

#endif
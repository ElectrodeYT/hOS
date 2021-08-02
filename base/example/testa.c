#include <stdio.h>
#include <unistd.h>
#include <ipc.h>

int main(int argc, char** argv) {
    uint8_t buffer[512];
    puts("Hello world from test a!\r\n");
    debug_printf("Process name: %s\r\n", argv[0]);
    ipchint(512, "helloworld");
    ipcrecv(buffer, 512, 1);
    debug_printf("Received message: %s\r\n", (char*)((uint64_t)buffer + 16));
    (void)argc;
    return 0;
}
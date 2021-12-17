#include <stdio.h>
#include <unistd.h>
#include <hos/ipc.h>

int main(int argc, char** argv) {
    uint8_t buffer[512];
    puts("Hello world from test a!\r\n");
    printf("Process name: %s\r\n", argv[0]);
    ipchint(512, "helloworld");
    ipcrecv(buffer, 512, 1);
    printf("Received message: %s\r\n", (char*)((uint64_t)buffer + 16));
    (void)argc;
    return 0;
}
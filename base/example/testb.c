#include <stdio.h>
#include <hos/ipc.h>
#include <string.h>

const char* ipc_msg = "Hello IPC World!";

int main() {
    puts("Hello C-World from test b!\r\n");
    while(ipcsendpipe(ipc_msg, strlen(ipc_msg) - 1, "helloworld") < 0);
    for(;;);
}
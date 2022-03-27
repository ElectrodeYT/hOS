#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __hos__
#error service is for hOS only
#endif

int main(int argc, char** argv) {
    printf("hello world!\n\r");
    printf("usermode, relocatable ELFs, go brr\n\r");
    // isatty test
    int isatty_test = isatty(0);
    printf("isatty ret: %i\n\r", isatty_test);
    perror("perror test");
    // open test
    int test_fd = open("/hello.txt");
    char buffer[101];
    int read_size = read(test_fd, buffer, 100);
    buffer[read_size] = '\0';
    printf("read test: %s\n\r", buffer);
    // fork test
    int pid = fork();
    if(pid == 0) {
        printf("should be forked\n\r");
        // read from fork fd=3
        int read_size = read(test_fd, buffer, 100);
        buffer[read_size] = '\0';
        printf("fork read test: %s\n\r", buffer);
        printf("exiting forked\n\r");
        exit(0);
    }
    // We waste a shit ton of time here
    for(size_t waste = 0; waste < 1000000; waste++) {
        buffer[waste % 100] = (((waste << 3) * 20) / 4) & 0xFF;
    }
    buffer[100] = '\0';
    // Now we write a lot
    printf("printf after fork exit: %s\n\r", buffer);
    for(;;);
}
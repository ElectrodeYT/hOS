#include <stdio.h>

#ifndef __hos__
#error service is for hOS only
#endif

int main(int argc, char** argv) {
    printf("hello world!\n\r");
    printf("usermode, relocatable ELFs, go brr\n\r");
    for(;;);
}
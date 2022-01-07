#include <stdio.h>

#ifndef __hos__
#error service is for hOS only
#endif

int main(int argc, char** argv) {
    printf("hello world!");
    for(;;);
}
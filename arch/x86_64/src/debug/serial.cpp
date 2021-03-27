#include <mem.h>
#include <hardware/instructions.h>
#include <debug/serial.h>

void __init_serial() {
    // i dont really understand this
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
    outb(0x3F8 + 4, 0x1E);
    outb(0x3F8 + 0, 0xAE);
    if(inb(0x3F8 + 0) != 0xAE) {
        return; // not gonna do much
    }
    outb(0x3F8 + 4, 0x0F);
}

namespace Kernel {
    namespace Debug {
        void SerialPutChar(const char c) {
            outb(0x3F8, c);
        }

        void SerialPrint(const char* c) {
            while(*c != '\0') {
                outb(0x3F8, *c++);
            }
        }
        void SerialPrintInt(long i, int base) {
            static char text[17] = "0123456789ABCDEF";
            char buf[32];
            int num = i;
            int pointer = 0; 

            for(int x = 0; x < 32; x++) { buf[x] = '\0'; }

            do {
            int modulo = num % base;
            if(modulo < 0) { modulo = -modulo; }
            buf[pointer++] = text[modulo];
            num /= base;
            } while(num != 0);

            // Print it backwards
            if(base == 10 && i < 0) { SerialPutChar('-'); }
            while(pointer >= 0) { SerialPutChar(buf[pointer--]); } 
            } 
    }
}
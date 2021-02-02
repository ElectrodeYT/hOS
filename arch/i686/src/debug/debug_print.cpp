#include <hardware/instructions.h>
#include <debug/debug_print.h>

namespace Kernel {
    void debug_put(char c) {
        outb(0x3F8, c);
    }

    void debug_puts(const char* s) {
        while(*s != '\0') {
            outb(0x3F8, *s++);
        }
    }

    void debug_serial_init() {
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

    void debug_puti(int i, int base) {
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
        if(base == 10 && i < 0) { debug_put('-'); }
        while(pointer >= 0) { debug_put(buf[pointer--]); } 
    }
}
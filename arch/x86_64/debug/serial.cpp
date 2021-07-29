#include <mem.h>
#include <panic.h>
#include <hardware/instructions.h>
#include <debug/serial.h>
#include <CPP/string.h>
#include <stdarg.h>

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
            char buf[32];
            itoa(i, buf, base);
            SerialPrint(buf);
        }

        void SerialPrintAddress(uint64_t i) {
            SerialPrint("0x");
            for(int nibble = 15; nibble >= 0; nibble--) {
                uint8_t val = (i & ((uint64_t)0xF << (nibble * 4))) >> (nibble * 4);
                SerialPutChar("0123456789ABCDEF"[val]);
            }
        }

        void SerialPrintf(char* str, ...) {
            va_list args;
            va_start(args, str);
            while(*str) {
                if(*str == '%') {
                    switch(*++str) {
                        case 0: ASSERT(false, "SerialPrintf: null terminator after %");
                        case '%': SerialPutChar('%'); break;
                        case 'i': 
                        case 'd': SerialPrintInt(va_arg(args, int), 10); break;
                        case 'l': SerialPrintInt(va_arg(args, long), 10); break;
                        case 'X':
                        case 'x': SerialPrintAddress(va_arg(args, long)); break;
                        case 's': SerialPrint(va_arg(args, char*)); break;
                        default: ASSERT(false, "SerialPrintf: invalid char after %");
                    }
                    str++;
                } else {
                    SerialPutChar(*str++);
                }
            }

            va_end(args);
        }
    }
}
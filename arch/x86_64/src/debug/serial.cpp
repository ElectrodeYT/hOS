#include <mem.h>
#include <debug/serial.h>

void __init_serial() {
    // lolcat lets hope stivale 
}

namespace Kernel {
    namespace Debug {
        void SerialPrint(const char* c) {
            (void)c;
        }
        void SerialPrintInt(long i) {
            (void)i;
        }
    }
}
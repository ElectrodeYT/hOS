#include <debug/serial.h>

namespace Kernel {
    namespace Debug {
        void __attribute__((noreturn)) Panic(const char* error) {
            Debug::SerialPrint("KERNEL PANIC: "); Debug::SerialPrint(error); Debug::SerialPrint("\n\rKernel halted.\n\r");
            for(;;) {
                asm volatile("cli; hlt"); 
            }
        }
    }
}
#include <debug/serial.h>

namespace Kernel {
    namespace Debug {
        void Panic(const char* error) {
            Debug::SerialPrint("KERNEL PANIC: "); Debug::SerialPrint(error); Debug::SerialPrint("\nKernel halted.\n");
            for(;;) {
                asm volatile("cli; hlt"); 
            }
        }
    }
}
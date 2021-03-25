#include <debug/serial.h>

namespace Kernel {
    namespace Panic {
        void Panic(char* error) {
            Debug::SerialPrint("KERNEL PANIC: "); Debug::SerialPrint(error); Debug::SerialPrint("\nKernel halted.\n");
            for(;;) {
                asm volatile("cli; hlt"); 
            }
        }
    }
}
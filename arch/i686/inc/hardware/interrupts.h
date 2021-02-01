#ifndef INTERRUPTS_H
#define INTERRUPTS_H

// Basic interrupt handling.
#include <stdint.h>
namespace Kernel {
    namespace Interrupts {

        struct __attribute__((packed)) Registers {
            uint32_t ds;					// Data segment selector
            uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; 	// Pushed by pusha.
            uint32_t int_no, err_code;			// Interrupt number and error code (if applicable)
            uint32_t eip, cs, eflags, useresp, ss;		// Pushed by the processor automatically.
        };
        void SetupInterrupts();

        void EnableInterrupts();
        void DisableInterrupts();
    }
}

#endif
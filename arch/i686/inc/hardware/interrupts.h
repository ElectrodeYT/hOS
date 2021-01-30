#ifndef INTERRUPTS_H
#define INTERRUPTS_H

// Basic interrupt handling.
#include <stdint.h>
namespace Kernel {
    namespace Interrupts {

        struct InterruptFrame {
            uint32_t ip;
            uint32_t cs;
            uint32_t flags;
            uint32_t sp;
            uint32_t ss;
        };

        void SetupInterrupts();

        void HandleInterrupt(int id, InterruptFrame* frame);

        void EnableInterrupts();
        void DisableInterrupts();
    }
}

#endif
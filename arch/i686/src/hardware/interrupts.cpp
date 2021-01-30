#include <hardware/interrupts.h>
#include <hardware/instructions.h>
#include <hardware/tables.h>

#include <debug/debug_print.h>

__attribute__((interrupt)) void isr_0(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(0, frame); }
__attribute__((interrupt)) void isr_1(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(1, frame); }
__attribute__((interrupt)) void isr_2(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(2, frame); }
__attribute__((interrupt)) void isr_3(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(3, frame); }
__attribute__((interrupt)) void isr_4(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(4, frame); }
__attribute__((interrupt)) void isr_5(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(5, frame); }
__attribute__((interrupt)) void isr_6(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(6, frame); }
__attribute__((interrupt)) void isr_7(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(7, frame); }
__attribute__((interrupt)) void isr_8(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(8, frame); }
__attribute__((interrupt)) void isr_9(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(9, frame); }
__attribute__((interrupt)) void isr_10(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(10, frame); }
__attribute__((interrupt)) void isr_11(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(11, frame); }
__attribute__((interrupt)) void isr_12(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(12, frame); }
__attribute__((interrupt)) void isr_13(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(13, frame); }
__attribute__((interrupt)) void isr_14(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(14, frame); }
__attribute__((interrupt)) void isr_15(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(15, frame); }

namespace Kernel {
    namespace Interrupts {
        void SetupInterrupts() {
            // Setup IDT
            IDT::InitIDT();

            // Set some irq's
            IDT::SetEntry(0, (uint32_t)isr_0, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(1, (uint32_t)isr_1, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(2, (uint32_t)isr_2, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(3, (uint32_t)isr_3, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(4, (uint32_t)isr_4, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(5, (uint32_t)isr_5, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(6, (uint32_t)isr_6, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(7, (uint32_t)isr_7, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(8, (uint32_t)isr_8, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(9, (uint32_t)isr_9, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(10, (uint32_t)isr_10, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(11, (uint32_t)isr_11, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(12, (uint32_t)isr_12, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(13, (uint32_t)isr_13, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(14, (uint32_t)isr_14, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(15, (uint32_t)isr_15, 0x8, IDT::InterruptGateType);

            IDT::ReloadIDT();
        }


        void HandleInterrupt(int id, InterruptFrame* frame) {
            debug_puts("Interrupt "); debug_puti(id); debug_puts("\n\r");
        }

        void EnableInterrupts() {
            asm volatile("sti");
        }

        void DisableInterrupts() {
            asm volatile("cli");
        }
    }
}
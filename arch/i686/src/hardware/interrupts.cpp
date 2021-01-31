#include <hardware/interrupts.h>
#include <hardware/instructions.h>
#include <hardware/tables.h>

#include <debug/debug_print.h>

#pragma region 
__attribute__((interrupt)) void isr_32(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(32, frame); }
__attribute__((interrupt)) void isr_33(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(33, frame); }
__attribute__((interrupt)) void isr_34(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(34, frame); }
__attribute__((interrupt)) void isr_35(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(35, frame); }
__attribute__((interrupt)) void isr_36(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(36, frame); }
__attribute__((interrupt)) void isr_37(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(37, frame); }
__attribute__((interrupt)) void isr_38(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(38, frame); }
__attribute__((interrupt)) void isr_39(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(39, frame); }
__attribute__((interrupt)) void isr_40(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(40, frame); }
__attribute__((interrupt)) void isr_41(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(41, frame); }
__attribute__((interrupt)) void isr_42(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(42, frame); }
__attribute__((interrupt)) void isr_43(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(43, frame); }
__attribute__((interrupt)) void isr_44(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(44, frame); }
__attribute__((interrupt)) void isr_45(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(45, frame); }
__attribute__((interrupt)) void isr_46(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(46, frame); }
__attribute__((interrupt)) void isr_47(Kernel::Interrupts::InterruptFrame* frame) { Kernel::Interrupts::HandleInterrupt(47, frame); }


#pragma endregion

namespace Kernel {
    namespace Interrupts {
        void SetupInterrupts() {
            // Setup IDT
            IDT::InitIDT();
            DisableInterrupts();
            // Set some irq's
            IDT::SetEntry(0, (uint32_t)isr_32, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(1, (uint32_t)isr_33, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(2, (uint32_t)isr_34, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(3, (uint32_t)isr_35, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(4, (uint32_t)isr_36, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(5, (uint32_t)isr_37, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(6, (uint32_t)isr_38, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(7, (uint32_t)isr_39, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(8, (uint32_t)isr_40, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(9, (uint32_t)isr_41, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(10, (uint32_t)isr_42, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(11, (uint32_t)isr_43, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(12, (uint32_t)isr_44, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(13, (uint32_t)isr_45, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(14, (uint32_t)isr_46, 0x8, IDT::InterruptGateType);
            IDT::SetEntry(15, (uint32_t)isr_47, 0x8, IDT::InterruptGateType);

            IDT::ReloadIDT();

            // Remap the PIC
            outb(0x20, 0x11);
            outb(0xA0, 0x11);
            outb(0x21, 0x20);
            outb(0xA1, 40);
            outb(0x21, 0x04);
            outb(0xA1, 0x02);
            outb(0x21, 0x01);
            outb(0xA1, 0x01);
            outb(0x21, 0x0);
            outb(0xA1, 0x0);
            EnableInterrupts();
        }


        __attribute__((optimize("O0"))) void HandleInterrupt(int id, InterruptFrame* frame) {
            // Check if this is a spurious interrupt
            if(id == 39) { // IRQ 7
                outb(0x20, 0x0B);
                uint8_t in_service_register = inb(0x20);
                // If bit7 isnt set, then this isnt a real interrupt and we can return
                if(!(in_service_register & 0x80)) { return; }
            }
            
            debug_puts("Interrupt "); debug_puti(id); debug_puts("\n\r");

            // Send the EOI if needed
            if(id >= 32 && id <= 47) {
                outb(0xA0, 0x20); // Master PIX EOI
                if(id >= 40) {
                    outb(0x20, 0x20); // Slave PIC EOI
                }
            } 
        }

        void EnableInterrupts() {
            asm volatile("sti");
        }

        void DisableInterrupts() {
            asm volatile("cli");
        }
    }
}
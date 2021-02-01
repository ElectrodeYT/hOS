#include <hardware/interrupts.h>
#include <hardware/instructions.h>
#include <hardware/tables.h>

#include <debug/debug_print.h>

#include <panic.h>

#pragma region 

// Interrupts from CPU (Page fault, GPF, ...)
extern "C" void isr0 ();
extern "C" void isr1 ();
extern "C" void isr2 ();
extern "C" void isr3 ();
extern "C" void isr4 ();
extern "C" void isr5 ();
extern "C" void isr6 ();
extern "C" void isr7 ();
extern "C" void isr8 ();
extern "C" void isr9 ();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr20();
extern "C" void isr21();
extern "C" void isr22();
extern "C" void isr23();
extern "C" void isr24();
extern "C" void isr25();
extern "C" void isr26();
extern "C" void isr27();
extern "C" void isr28();
extern "C" void isr29();
extern "C" void isr30();
extern "C" void isr31();

// Interrupt requests from hardware
extern "C" void irq0();
extern "C" void irq1();
extern "C" void irq2();
extern "C" void irq3();
extern "C" void irq4();
extern "C" void irq5();
extern "C" void irq6();
extern "C" void irq7();
extern "C" void irq8();
extern "C" void irq9();
extern "C" void irq10();
extern "C" void irq11();
extern "C" void irq12();
extern "C" void irq13();
extern "C" void irq14();
extern "C" void irq15();

#pragma endregion

namespace Kernel {
    namespace Interrupts {
        void SetupInterrupts() {
            // Setup IDT
            IDT::InitIDT();
            DisableInterrupts();


            // IRQs
            IDT::SetEntry(32, (uint32_t)irq0, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(33, (uint32_t)irq1, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(34, (uint32_t)irq2, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(35, (uint32_t)irq3, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(36, (uint32_t)irq4, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(37, (uint32_t)irq5, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(38, (uint32_t)irq6, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(39, (uint32_t)irq7, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(40, (uint32_t)irq8, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(41, (uint32_t)irq9, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(42, (uint32_t)irq10, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(43, (uint32_t)irq11, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(44, (uint32_t)irq12, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(45, (uint32_t)irq13, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(46, (uint32_t)irq14, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(47, (uint32_t)irq15, 0x08, IDT::InterruptGateType);

            // ISRs
            IDT::SetEntry(0, (uint32_t)isr0, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(1, (uint32_t)isr1, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(2, (uint32_t)isr2, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(3, (uint32_t)isr3, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(4, (uint32_t)isr4, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(5, (uint32_t)isr5, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(6, (uint32_t)isr6, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(7, (uint32_t)isr7, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(8, (uint32_t)isr8, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(9, (uint32_t)isr9, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(10, (uint32_t)isr10, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(11, (uint32_t)isr11, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(12, (uint32_t)isr12, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(13, (uint32_t)isr13, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(14, (uint32_t)isr14, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(15, (uint32_t)isr15, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(16, (uint32_t)isr16, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(17, (uint32_t)isr17, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(18, (uint32_t)isr18, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(19, (uint32_t)isr19, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(20, (uint32_t)isr20, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(21, (uint32_t)isr21, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(22, (uint32_t)isr22, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(23, (uint32_t)isr23, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(24, (uint32_t)isr24, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(25, (uint32_t)isr25, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(26, (uint32_t)isr26, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(27, (uint32_t)isr27, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(28, (uint32_t)isr28, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(29, (uint32_t)isr29, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(30, (uint32_t)isr30, 0x08, IDT::InterruptGateType);
            IDT::SetEntry(31, (uint32_t)isr31, 0x08, IDT::InterruptGateType);

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
        }

        extern "C" void isr_handler(Registers r) {
            switch(r.int_no) {
                case 0:
                case 2:
                case 4:
                case 5:
                case 6:
                case 8:
                case 10:
                case 11:
                case 12:
                case 16:
                case 17:
                case 18:
                case 19:
                case 20:
                case 30: {
                    debug_puts("Unrecoverable exception: "); debug_puti(r.int_no);
                    panic("Unrecoverable exception");
                }
                case 14: {
                    debug_puts("Page fault: ");
                    if(r.err_code & 0b1) { debug_puts("P "); }
                    if(r.err_code & 0b10) { debug_puts("W "); } else { debug_puts("R "); }
                    if(r.err_code & 0b100) { debug_puts("U "); }
                    if(r.err_code & 0b1000) { debug_puts("R "); }
                    if(r.err_code & 0b10000) { debug_puts("I "); }
                    uint32_t cr2;
                    asm volatile("movl %%cr2, %0" : "=a" (cr2));
                    debug_puti(cr2, 16);
                    panic("Unrecoverable page fault");
                }

                default: {
                    panic("Unhandled exception");
                }
            }
        }

        extern "C" void irq_handler(Registers r) {
            switch(r.int_no) {
                case 0: {
                    // TODO: Scheduling
                    break;
                }
                default: {
                    panic("Unhandled IRQ");
                }
            }
            // Send EOI
            if(r.int_no >= 40) {
                // Send slave EOI
                outb(0xA0, 0x20);
            }
            outb(0x20, 0x20);
        }

        void EnableInterrupts() {
            asm volatile("sti");
        }

        void DisableInterrupts() {
            asm volatile("cli");
        }
    }
}
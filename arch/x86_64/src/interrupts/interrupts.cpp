#include <mem.h>
#include <hardware/instructions.h>
#include <interrupts/interrupts.h>
#include <panic.h>

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



// TODO
extern "C" void isr_main(Kernel::Interrupts::ISRRegisters* registers) {
    Kernel::Debug::Panic("ISR stub");
    (void)registers;
}

// TODO
extern "C" void irq_main(Kernel::Interrupts::ISRRegisters* registers) {
    Kernel::Debug::Panic("IRQ stub");
    (void)registers;
}

namespace Kernel {
    namespace Interrupts {

        struct IDTPointer {
            uint16_t size;
            uint64_t base;
        }__attribute__((packed));

        static IDTDescr idt[256] __attribute__((section(".idt")));
        static IDTPointer idt_pointer;

        void CreateEntry(int entry, void(*handler)(), uint8_t options) {
            idt[entry].offset_1 = (uint64_t)handler & 0xFFFF;
            idt[entry].offset_2 = ((uint64_t)handler >> 16) & 0xFFFF;
            idt[entry].offset_3 = ((uint64_t)handler >> 32) & 0xFFFFFFFF;
            idt[entry].ist = 0;
            idt[entry].selector = 0x8;
            idt[entry].type_attr = options;
            idt[entry].zero = 0;
        }

        void InitInterrupts() {
            // Remap the PIC
            // This should map all IRQs beginning at 32 dec.
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

            // Create all the IDT entries
            memset((void*)idt, 0x00, sizeof(idt));

            // Interrupts
            CreateEntry(0, isr0, InterruptGate);
            CreateEntry(1, isr1, InterruptGate);
            CreateEntry(2, isr2, InterruptGate);
            CreateEntry(3, isr3, InterruptGate);
            CreateEntry(4, isr4, InterruptGate);
            CreateEntry(5, isr5, InterruptGate);
            CreateEntry(6, isr6, InterruptGate);
            CreateEntry(7, isr7, InterruptGate);
            CreateEntry(8, isr8, InterruptGate);
            CreateEntry(9, isr9, InterruptGate);
            CreateEntry(10, isr10, InterruptGate);
            CreateEntry(11, isr11, InterruptGate);
            CreateEntry(12, isr12, InterruptGate);
            CreateEntry(13, isr13, InterruptGate);
            CreateEntry(14, isr14, InterruptGate);
            CreateEntry(15, isr15, InterruptGate);
            CreateEntry(16, isr16, InterruptGate);
            CreateEntry(17, isr17, InterruptGate);
            CreateEntry(18, isr18, InterruptGate);
            CreateEntry(19, isr19, InterruptGate);
            CreateEntry(20, isr20, InterruptGate);
            CreateEntry(21, isr21, InterruptGate);
            CreateEntry(22, isr22, InterruptGate);
            CreateEntry(23, isr23, InterruptGate);
            CreateEntry(24, isr24, InterruptGate);
            CreateEntry(25, isr25, InterruptGate);
            CreateEntry(26, isr26, InterruptGate);
            CreateEntry(27, isr27, InterruptGate);
            CreateEntry(28, isr28, InterruptGate);
            CreateEntry(29, isr29, InterruptGate);
            CreateEntry(30, isr30, InterruptGate);
            CreateEntry(31, isr31, InterruptGate);

            // IRQs
            CreateEntry(32, irq0, InterruptGate);
            CreateEntry(33, irq1, InterruptGate);
            CreateEntry(34, irq2, InterruptGate);
            CreateEntry(35, irq3, InterruptGate);
            CreateEntry(36, irq4, InterruptGate);
            CreateEntry(37, irq5, InterruptGate);
            CreateEntry(38, irq6, InterruptGate);
            CreateEntry(39, irq7, InterruptGate);
            CreateEntry(40, irq8, InterruptGate);
            CreateEntry(41, irq9, InterruptGate);
            CreateEntry(42, irq10, InterruptGate);
            CreateEntry(43, irq11, InterruptGate);
            CreateEntry(44, irq12, InterruptGate);
            CreateEntry(45, irq13, InterruptGate);
            CreateEntry(46, irq14, InterruptGate);
            CreateEntry(47, irq15, InterruptGate);

            // Setup IDT pointer
            idt_pointer.base = (uint64_t)&idt;
            idt_pointer.size = sizeof(idt);

            // Load it
            asm volatile("lidt %0" : : "m" (idt_pointer));
        }
    }
}
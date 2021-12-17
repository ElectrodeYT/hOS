#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
namespace Kernel {
   class Interrupts {
   public:
      struct ISRRegisters {
         // CPU Registers
         uint64_t r15;
         uint64_t r14;
         uint64_t r13;
         uint64_t r12;
         uint64_t r11;
         uint64_t r10;
         uint64_t r9;
         uint64_t r8;
         uint64_t rax;
         uint64_t rbx;
         uint64_t rcx;
         uint64_t rdx;
         uint64_t rsi;
         uint64_t rdi;
         uint64_t rbp;
         // Contains interrupt id
         uint64_t int_num;
         // Contains error code
         uint64_t error;
         // Interrupt stack frame
         uint64_t rip;
         uint64_t cs;
         uint64_t rflags;
         uint64_t rsp;
         uint64_t ss;
      }__attribute__((packed));

      // Init the interrupts
      void InitInterrupts();

      // Interrupt handlers
      typedef void (*irq_handler_t)(struct ISRRegisters*);
      void RegisterIRQHandler(int irq_number, irq_handler_t handler);

      // Proper Interrupt handlers
      void HandleISR(ISRRegisters* registers);
      void HandleIRQ(ISRRegisters* registers);

      // Singleton
      static Interrupts& the() {
         static Interrupts instance;
         return instance;
      }



   private:

      // Registered IRQ Handlers
      irq_handler_t irq_handlers[16];


      // PIC IO addresses
      const uint16_t pic1_io_command = 0x20;
      const uint16_t pic1_io_data = 0x21;
      const uint16_t pic2_io_command = 0xA0;
      const uint16_t pic2_io_data = 0xA1;

      // A descriptor in the IDT
      struct IDTDescr {
         uint16_t offset_1; // offset bits 0..15
         uint16_t selector; // a code segment selector in GDT or LDT
         uint8_t ist;       // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
         uint8_t type_attr; // type and attributes
         uint16_t offset_2; // offset bits 16..31
         uint32_t offset_3; // offset bits 32..63
         uint32_t zero;     // reserved
      }__attribute__((packed));


      // Interrupt gate type
      const uint8_t InterruptGate = 0b10001110;

      // Exception numbers
      enum ExceptionID {
         DivideByZero = 0,
         Debug,
         NMI,
         Breakpoint,
         Overflow,
         BoundRangeExceeded,
         InvalidOpcode,
         DeviceNotAvailable,
         DoubleFault,
         CoprocessorSegmentOverrun,
         InvalidTSS,
         SegmentNotPresent,
         StackSegmentFault,
         GeneralProtectionFault,
         PageFault,
         x87FloatingPointException = 16,
         AligmentCheck,
         MachineCheck,
         SIMDFloatingPointException,
         VirtualizationException,
         SecurityException = 30
      };
   
      struct IDTPointer {
         uint16_t size;
         uint64_t base;
      }__attribute__((packed));

      IDTDescr idt[256];
      IDTPointer idt_pointer;

      void CreateEntry(int entry, void(*handler)(), uint8_t options);
   };
}
#endif

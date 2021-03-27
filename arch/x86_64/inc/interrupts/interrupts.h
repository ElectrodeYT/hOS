#ifndef INTERRUPTS_H
#define INTERRUPTS_h

#include <stdint.h>
namespace Kernel {
   namespace Interrupts {
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

      struct ISRRegisters {
         // CPU Registers
         uint64_t r11;
         uint64_t r10;
         uint64_t r9;
         uint64_t r8;
         uint64_t rax;
         uint64_t rcx;
         uint64_t rdx;
         uint64_t rsi;
         uint64_t rdi;
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

      // Interrupt gate type
      const uint8_t InterruptGate = 0b10001110;
      void InitInterrupts();

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

      // Interrupt handlers
      typedef void (*irq_handler_t)(struct ISRRegisters*);
      void RegisterIRQHandler(int irq_number, irq_handler_t handler);


      void HandleISR(ISRRegisters* registers);
      void HandleIRQ(ISRRegisters* registers);
   }
}
#endif
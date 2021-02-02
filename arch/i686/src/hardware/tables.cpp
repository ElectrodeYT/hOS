#include <stdint.h>
#include <hardware/tables.h>
#include <memory/heap.h>
#include <memory/memorymanager.h>

namespace Kernel {
    namespace GDT {
        struct __attribute__((packed)) __attribute__((aligned(0x1000))) GDTPointer {
            volatile uint16_t size;
            volatile uint32_t adr;
        };

        struct __attribute__((packed)) GDTDesc {
            volatile uint16_t limit_0 = 0;
            volatile uint16_t base_0 = 0;
            volatile uint8_t base_1 = 0;
            volatile uint8_t access = 0;
            volatile uint8_t flags_limit_1 = 0;
            volatile uint8_t base_2 = 0;
        };

        struct __attribute__((packed)) __attribute__((aligned(0x1000))) GDT {
            GDTDesc null;
            GDTDesc KernelCode;
            GDTDesc KernelData;
            GDTDesc UserCode;
            GDTDesc UserData;
            GDTDesc TSS;
        };
        static volatile GDT gdt;
        static volatile GDTPointer gdt_pointer;


        __attribute__((optimize("O0")))  void SetSegment(volatile GDTDesc* desc, uint32_t limit, uint32_t base, uint8_t type) {
            // Calculate granularity and adjust limit
            if(limit > 65536) {
                desc->flags_limit_1 = 0xC0;
                limit = limit >> 12;
            } else if(limit != 0) {
                desc->flags_limit_1 = 0x40;
            }
            desc->limit_0 = limit & 0xFFFF;
            desc->flags_limit_1 |= (limit >> 16) & 0x0F;
            desc->base_0 = base & 0xFFFF;
            desc->base_1 = (base >> 16) & 0xFF;
            desc->base_2 = (base >> 24) & 0xFF;
            desc->access = type;
        }


        __attribute__((optimize("O0")))  void InitGDT() {
            // Allocate gdt and gdt_pointer
            gdt_pointer.adr = (uint32_t)&gdt;
            gdt_pointer.size = sizeof(GDT) - 1;

            // Set the segements
            SetSegment(&(gdt.null), 0, 0, 0);
            SetSegment(&(gdt.KernelCode), 0xffffffff, 0, 0x9A);
            SetSegment(&(gdt.KernelData), 0xffffffff, 0, 0x92);
            SetSegment(&(gdt.UserCode), 0xffffffff, 0, 0xFA);
            SetSegment(&(gdt.UserData), 0xffffffff, 0, 0xF2);
            SetSegment(&(gdt.TSS), 0, 0, 0);
            
            // Some inline assembly to load the GDT
            asm volatile("lgdt %0" : : "m"(gdt_pointer));
            flush_gdt();
        }
    }
    namespace IDT {
        struct __attribute__((packed)) __attribute__((aligned(0x1000))) IDTPointer {
            uint16_t size;
            uint32_t adr;
        };

        IDTPointer idt_pointer;

        __attribute__((aligned(0x1000))) uint8_t idt[8 * 256];

        void SetEntry(uint32_t entry, uint32_t offset, uint16_t segment, uint8_t type) {
            idt[entry * 8 + 0] = offset & 0xFF;
            idt[entry * 8 + 1] = (offset >> 8) & 0xFF;
            idt[entry * 8 + 2] = segment & 0xFF;
            idt[entry * 8 + 3] = (segment >> 8) & 0xFF;
            idt[entry * 8 + 4] = 0;
            idt[entry * 8 + 5] = type;
            idt[entry * 8 + 6] = (offset >> 16) & 0xFF;
            idt[entry * 8 + 7] = (offset >> 24) & 0xFF;
        }

        void ReloadIDT() {
            asm volatile("lidt %0" : : "m"(idt_pointer));
        }

        void InitIDT() {
            // Clear IDT
            for(int i = 0; i < 8 * 256; i++) { idt[i] = 0; }
            // Set idt pointer
            idt_pointer.adr = (uint32_t)idt;
            idt_pointer.size = 8 * 256 - 1;
            // Some inline assembly to load the IDT
            asm volatile("lidt %0" : : "m"(idt_pointer));
        }
    }
    namespace TSS {
        struct __attribute__((packed))  __attribute__((aligned(0x1000))) TSS {
            uint16_t link; uint16_t res_1;
            uint32_t esp0;
            uint16_t ss0; uint16_t res_2;
            uint32_t esp1;
            uint16_t ss1; uint16_t res_3;
            uint32_t esp2;
            uint16_t ss2; uint16_t res_4;
            uint32_t cr3;
            uint32_t eip;
            uint32_t eflags;
            uint32_t eax;
            uint32_t ecx;
            uint32_t edx;
            uint32_t ebx;
            uint32_t esp;
            uint32_t ebp;
            uint32_t esi;
            uint32_t edi;
            uint16_t es; uint16_t res_5;
            uint16_t cs; uint16_t res_6;
            uint16_t ss; uint16_t res_7;
            uint16_t ds; uint16_t res_8;
            uint16_t fs; uint16_t res_9;
            uint16_t gs; uint16_t res_10;
            uint16_t ldts; uint16_t res_11;
            uint16_t res_12; uint16_t iopb_offset;            
        };

        static TSS tss;

        void InitTSS() {
            // Set some shit
            tss.ss0 = 0x10;
            tss.iopb_offset = 104;
            // We allocate a new stack for interrupts
            // We dont really care where it is in virtual memory, so we dont save it
            tss.esp0 = MemoryManager::VirtualMemory::GetPage().virt;
            GDT::SetSegment(&(GDT::gdt.TSS), 104, (uint32_t)&tss, 0x89);
            asm volatile("ltr %%ax" : : "a"(0x28));
        }
    }
}
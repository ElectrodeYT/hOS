#include <stdint.h>
#include <hardware/tables.h>
#include <memory/heap.h>
#include <memory/memorymanager.h>

namespace Kernel {
    namespace GDT {

        struct __attribute__((packed)) GDTPointer {
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

        static volatile GDTDesc gdt[16] __attribute__((packed)) __attribute__((aligned(0x1000)));

        static volatile GDTPointer* gdt_pointer;

        static uint32_t next_gdt_entry = 0;

        void AddSegment(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
            gdt[next_gdt_entry].limit_0 = limit & 0xFFFF;
            gdt[next_gdt_entry].base_0 = base & 0xFFFF;
            gdt[next_gdt_entry].base_1 = (base >> 16) & 0xFF;
            gdt[next_gdt_entry].access = access;
            gdt[next_gdt_entry].flags_limit_1 = ((limit >> 16) & 0x0F) | (flags & 0xF0);
            gdt[next_gdt_entry].base_2 = (base >> 24) & 0xFF;
            next_gdt_entry++;
        }

        void InitGDT() {
            // Allocate gdt and gdt_pointer

            gdt_pointer = new GDTPointer;
            gdt_pointer->adr = (uint32_t)gdt;
            gdt_pointer->size = sizeof(GDTDesc) * 16 - 1;

            // Create really needed segments
            AddSegment(0, 0, 0, 0);
            AddSegment(0, 0xFFFFFFFF, 0x9A, 0xC0); // Kernel Code
            AddSegment(0, 0xFFFFFFFF, 0x92, 0xC0); // Kernel Data
            // Some inline assembly to load the GDT
            asm volatile("lgdt %0" : : "m"(gdt_pointer));
            flush_gdt();
        }
    }
    namespace IDT {
        struct __attribute__((packed)) IDTPointer {
            uint16_t size;
            uint32_t adr;
        };

        IDTPointer* idt_pointer;

        uint8_t* idt;

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
            // Allocate IDT
            idt = new uint8_t[8 * 256];
            for(int i = 0; i < 8 * 256; i++) { idt[i] = 0; }
            // Allocate IDT pointer
            idt_pointer = new IDTPointer;
            // Set idt pointer
            idt_pointer->adr = (uint32_t)idt;
            idt_pointer->size = 8 * 256;
            // Some inline assembly to load the IDT
            asm volatile("lidt %0" : : "m"(idt_pointer));
        }
    }
}
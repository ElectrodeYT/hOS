#ifndef TABLES_H
#define TABLES_H

#include <stdint.h>

// GDT and IDT and TSS
namespace Kernel {
    namespace GDT {
        void AddSegment(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
        void InitGDT();
        extern "C" void flush_gdt();
    }
    namespace IDT {
        void SetEntry(uint32_t entry, uint32_t offset, uint16_t segment, uint8_t type);
        void InitIDT();
        void ReloadIDT();
        const uint8_t InterruptGateType = 0b10001110;
    }
    namespace TSS {
        void InitTSS();
    }
}

#endif
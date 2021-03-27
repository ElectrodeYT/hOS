#ifndef VIRTMEM_H
#define VIRTMEM_H

#include <stddef.h>

namespace Kernel {
    namespace VirtualMemory {
        // Initialize virtual memory.
        // Also creates the page tables, and changes to them.
        void Init();

        void SwitchPageTables();

        void* AllocatePages(size_t count = 1);
        void FreePages(void* adr, int count);

        // Maps addresses.
        void MapPage(unsigned long phys, unsigned long virt, unsigned long options = 0b11);
        // Modified version of __virtmem_map that uses stiavle2's identity mapping to create new pages tables.
        void MapPageEarly(unsigned long phys, unsigned long virt);
    }
}

#endif
#ifndef VIRTMEM_H
#define VIRTMEM_H

#include <stddef.h>

namespace Kernel {
    namespace VirtualMemory {
        const uint64_t kernel_page_begin = 0xffffffff90000000;
        const uint64_t kernel_page_end = 0xfffffffffffff000;

        // Initialize virtual memory.
        // Also creates the page tables, and changes to them.
        void Init();

        void SwitchPageTables();
        // Sets CR3 to table. Must be a physical address.
        void SwitchPageTables(uint64_t table);

        void* AllocatePages(size_t count = 1);
        void FreePages(void* adr, int count);

        // Maps addresses.
        void MapPage(unsigned long phys, unsigned long virt, unsigned long options = 0b11);
        // Modified version of __virtmem_map that uses stiavle2's identity mapping to create new pages tables.
        void MapPageEarly(unsigned long phys, unsigned long virt);
        // Create a new page table. Returns physical address. Keeps kernel mappings in entry 511.
        uint64_t CreateNewPageTable();
        // Tag a level 3 table as global
        void TagLevel3Global(unsigned long entry);
    }
}

#endif
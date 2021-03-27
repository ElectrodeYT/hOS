#ifndef VIRTMEM_H
#define VIRTMEM_H

#include <stddef.h>

namespace Kernel {
    class VirtualMemory {
    public:
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
        // Gets a physical address
        uint64_t GetPhysical(uint64_t virt);

        // Singleton
        static VirtualMemory& the() {
            static VirtualMemory instance;
            return instance;
        }


    private:
        void InvalidatePage(uint64_t address);
        
        uint64_t* CalculateRecursiveLevel1(unsigned long lvl2, unsigned long lvl3, unsigned long lvl4, unsigned long offset = 0);
        uint64_t* CalculateRecursiveLevel2(unsigned long lvl3, unsigned long lvl4, unsigned long offset = 0);
        uint64_t* CalculateRecursiveLevel3(unsigned long lvl4, unsigned long offset = 0);
        uint64_t* CalculateRecursiveLevel4(unsigned long offset = 0);
        

        // Contains the physical address of the original page table.
        uint64_t* page_table;

        // Define Kernel page area.
        uint8_t* kernel_page_bitmap;

    };
}

#endif
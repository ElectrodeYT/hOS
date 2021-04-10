#ifndef VIRTMEM_H
#define VIRTMEM_H

#include <stddef.h>
#include <CPP/vector.h>

namespace Kernel {
    namespace VM {
        class Manager {
        public:
            const uint64_t kernel_page_begin = 0xffffffff90000000;
            const uint64_t kernel_page_end = 0xfffffffffffff000;

            // Virtual class for other VM objects
            struct VMObject {
                VMObject(bool a, bool s) : _allocated(a), _shared(s) { }
                bool isAllocated() { return _allocated; }
                bool isShared() { return _shared; }

                uint64_t base;
                uint64_t size;
            private:
                bool _allocated;
                bool _shared;
            };
            

            // Initialize virtual memory.
            // Also creates the page tables, and changes to them.
            void Init();

            void SwitchPageTables();
            // Sets CR3 to table. Must be a physical address.
            void SwitchPageTables(uint64_t table);
            // Gets the value of CR3
            uint64_t CurrentPageTable();

            // Allocate a virtual memory page. Allocated into Kernel memory.
            void* AllocatePages(size_t count = 1, unsigned long page_options = 0b11, bool shared = false);


            void FreePages(void* adr);

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
            static Manager& the() {
                static Manager instance;
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

            // Vector of all allocated memory
            Vector<VMObject*> mem;
        };
    }
}

#endif
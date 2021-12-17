#ifndef VIRTMEM_H
#define VIRTMEM_H

#include <stddef.h>
#include <CPP/vector.h>
#include <CPP/mutex.h>
#include <debug/serial.h>

constexpr uint64_t round_to_page_up(uint64_t addr) {
    return ((addr + 4096 - 1) & ~(4096 - 1));
}

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

                // Make this VMObject shared.
                // A Region cannot be converted from shared to private, so this is
                // the only way to modify _shared after the fact.
                void makeShared() { _shared = true; }

                bool read = true;
                bool write = true;
                bool execute = true;

                uint64_t base;
                uint64_t size;

                uint8_t checkCopyOnWrite(uint64_t page) {
                    if(!ref_counts) { return 0; }
                    return ref_counts[page];
                }

                void decrementCopyOnWrite(uint64_t page) {
                    if(ref_counts) {
                        if(ref_counts[page] > 0) { ref_counts[page]--; }
                    }
                }

                ~VMObject() {
                    // Decrement all CoW
                    if(ref_counts) {
                        bool all_are_zero = true;
                        for(size_t i = 0; i < size; i += 4096) {
                            if(ref_counts[i / 4096] > 0) { ref_counts[i / 4096]--; }
                            if(ref_counts[i / 4096] != 0 ) { all_are_zero = false; }
                        }
                        if(all_are_zero) {
                            // Check if all the refcounts are gone
                            all_are_zero = true;
                            for(size_t i = 0; i < ref_count_org_base_len; i++) {
                                if(ref_count_org_base[i] != 0) { all_are_zero = false; break; }
                            }
                            if(all_are_zero) {
                                // Debug::SerialPrintf("ref_count_org_base for mapping at %x has depleted, deleteing ref_counts_org_base\r\n", base);
                                delete ref_count_org_base;
                            }
                        }
                    }
                }

                // This creates a copy of this mapping as a CoW
                // copy.
                // This marks all pages as CoW. It is assumed that
                // the correct page table for this VMObject is currently
                // in use. The page table mappings for the new VMObject
                // must be created manually. All pages should be mapped
                // as read-only.
                // This can only be used on user VMObjects.
                VMObject* copyAsCopyOnWrite(uint64_t* phys_addresses) {
                    if(!ref_counts) {
                        ref_counts = new uint8_t[round_to_page_up(size) / 4096];
                        memset(ref_counts, 1, round_to_page_up(size) / 4096);
                        ref_count_org_base = ref_counts;
                        ref_count_org_base_len = round_to_page_up(size) / 4096;
                    }
                    for(size_t i = 0; i < size; i += 4096) {
                        ref_counts[i / 4096]++;
                        uint64_t phys = VM::Manager::the().GetPhysical(base + i);
                        if(phys_addresses) { phys_addresses[i / 4096] = phys; }
                        VM::Manager::the().MapPage(phys, base + i, 0b101);
                    }
                    VMObject* new_obj = new VMObject(_allocated, _shared);
                    new_obj->base = base;
                    new_obj->size = size;
                    new_obj->ref_counts = ref_counts;
                    new_obj->read = read;
                    new_obj->write = write;
                    new_obj->execute = execute;
                    return new_obj;
                }

                // Refcount for this region.
                // Only for shared regions.
                uint64_t refcount = 1;
            private:
                bool _allocated;
                bool _shared;

                // CoW ref counters
                uint8_t* ref_counts = NULL;
                // Original base of the allocated ref counts
                // This is needed for munmap, as it might split
                // a ref counted vmobject and it might be able to
                // reallocate this
                uint8_t* ref_count_org_base = NULL;
                uint64_t ref_count_org_base_len = 0;
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

            // Mutex
            mutex_t mutex;
        };
    }
}

#endif
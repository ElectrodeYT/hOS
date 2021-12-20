#include <mem.h>
#include <mem/VM/virtmem.h>
#include <mem/PM/physalloc.h>
#include <panic.h>

namespace Kernel {
    namespace VM {
        // Return the addresses of these tables
        uint64_t* Manager::CalculateRecursiveLevel1(unsigned long lvl2, unsigned long lvl3, unsigned long lvl4, unsigned long offset) {
            return (uint64_t*)(0xFFFFF80000000000 | ((lvl4 & 0x1FF) << 30) | ((lvl3 & 0x1FF) << 21) | ((lvl2 & 0x1FF) << 12) | (offset & 0x1FFF));
        }

        uint64_t* Manager::CalculateRecursiveLevel2(unsigned long lvl3, unsigned long lvl4, unsigned long offset) {
            return (uint64_t*)(0xFFFFF87C00000000 | (lvl4 << 21) | (lvl3 << 12) | (offset & 0x1FFF));
        }

        uint64_t* Manager::CalculateRecursiveLevel3(unsigned long lvl4, unsigned long offset) {
            return (uint64_t*)(0xFFFFF87C3E000000 | (lvl4 << 12) | (offset & 0x1FFF));
        }

        uint64_t* Manager::CalculateRecursiveLevel4(unsigned long offset) {
            // This one is simple
            return (uint64_t*)(0xFFFFF87C3E1F0000 | (offset & 0x1FFF));
        }

        void Manager::TagLevel3Global(unsigned long entry) {
            // Check if the level 3 table exists
            uint64_t* lvl4_table = Manager::CalculateRecursiveLevel4(0);
            if(~(lvl4_table[entry]) & 1) { return;}
            lvl4_table[entry] |= (1 << 8);
        }

        void Manager::InvalidatePage(uint64_t address) {
            asm volatile("invlpg (%0)" : : "b"(address) : "memory");
            //SwitchPageTables(); // lol
            (void)address;
        }

        // Get the physical address of a mapping. Returns NULL if failed.
        uint64_t Manager::GetPhysical(uint64_t virt) {
            int lvl4 = (virt >> 39) & 0b111111111;
            int lvl3 = (virt >> 30) & 0b111111111;
            int lvl2 = (virt >> 21) & 0b111111111;
            int lvl1 = (virt >> 12) & 0b111111111;
            // Check if the level 3 table exists
            uint64_t* lvl4_table = Manager::CalculateRecursiveLevel4(0);
            if(~(lvl4_table[lvl4]) & 1) { return (uint64_t)NULL; }
            uint64_t* lvl3_table = Manager::CalculateRecursiveLevel3(lvl4);
            // Check if the level 2 table exists
            if(~(lvl3_table[lvl3]) & 1) { return (uint64_t)NULL; }
            uint64_t* lvl2_table = Manager::CalculateRecursiveLevel2(lvl3, lvl4);
            // Check if the level 1 table exists
            if(~(lvl2_table[lvl2]) & 1) { return (uint64_t)NULL; }
            uint64_t* lvl1_table = Manager::CalculateRecursiveLevel1(lvl2, lvl3, lvl4);
            // Check the entry
            uint64_t entry = lvl1_table[lvl1];
            if(~(entry) & 1) { return (uint64_t)NULL; }
            return entry & 0xFFFFFFFFFF000;
        } 

        void Manager::Init() {
            // We are still using stivale2's memory mapping, so we can just write to the pages allocated by the physical memory allocator.
            // We want to change as quickly as possible, so we just create the basic kernel mappings and go.
            // This will however be done by the _start function.
            page_table = (uint64_t*)PM::Manager::the().AllocatePages();
            memset((void*)page_table, 0x00, 512 * 8);

            // Setup recursive mapping
            // stivale2 maps the kernel into a location requiring the last page,
            // so we cant really map it there.
            // We set the 0x1F0 entry to itself instead.
            page_table[0x1F0] = (uint64_t)page_table | 0b11; // Present, writeable

            // Create the kernel memory object
            VMObject* vm = new VMObject(false, false);
            vm->base = kernel_page_begin;
            vm->size = kernel_page_end - kernel_page_begin;
            mem.push_back(vm);
        }

        void Manager::SwitchPageTables() {
            // Change CR3 register to page_table
            // This should auto-flush the TLB
            asm volatile("mov %0, %%cr3" : :"r"(page_table));
        }

        // Set the page table to a specific table.
        void Manager::SwitchPageTables(uint64_t table) {
            asm volatile("mov %0, %%cr3" : : "r"(table));
        }

        uint64_t Manager::CurrentPageTable() {
            uint64_t ret;
            asm volatile("mov %%cr3, %0" : "=r"(ret));
            return ret;
        }

        void* Manager::AllocatePages(size_t count, unsigned long options, bool shared) {
            acquire(&mutex);
            // Get new physical page
            uint64_t physical = PM::Manager::the().AllocatePages(count);
            // Go through mem to find new memory
            for(size_t i = 0; i < mem.size(); i++) {
                VMObject* curr = mem.at(i);
                if(curr->isAllocated() == false) {
                    // Unallocated, check if its big enough
                    size_t byte_count = count * 4096;
                    if(curr->size > byte_count) {
                        // Its big enough, create new object
                        VMObject* new_object = new VMObject(true, shared);
                        new_object->base = curr->base;
                        new_object->size = byte_count;
                        // Size down curr
                        curr->base += byte_count;
                        curr->size -= byte_count;
                        if(curr->size == 0) {
                            // TODO: this
                        }
                        mem.insert(i, new_object);
                        // Map new_object
                        for(size_t i = 0; i < count; i++) {
                            uint64_t map_phys = physical + (i  * 4096);
                            uint64_t map_virt = new_object->base + (i * 4096);
                            MapPage(map_phys, map_virt, options);
                        }
                        release(&mutex);
                        return (void*)new_object->base;
                    }
                }
            }
            release(&mutex);
            return NULL; // No memory available
        }

        // TODO
        void Manager::FreePages(void* adr) {
            // Check if this is a user or kernel address
            if((uint64_t)adr & (1UL << 63)) {
                // Is a kernel address
                for(size_t i = 0; i < mem.size(); i++) {
                    VMObject* curr = mem.at(i);
                    if(curr->isAllocated() && curr->base == (uint64_t)adr) {
                        VMObject* new_unalloc = new VMObject(false, false);
                        new_unalloc->base = curr->base;
                        new_unalloc->size = curr->size;
                        // Debug::SerialPrintf("Deallocating kernel pages %x, size %x\r\n", curr->base, curr->size);
                        delete curr;
                        mem.remove(i);
                        mem.push_back(new_unalloc);
                        // TODO: merge
                        return;
                    }
                }
            } else {
                // Is a user address
                return;
            }
        }



        // Create a new page table.
        uint64_t Manager::CreateNewPageTable() {
            // acquire(&mutex);
            // Get entry for the kernel mappings
            uint64_t kernel_mappings = CalculateRecursiveLevel4()[511];
            uint64_t* new_table_virtual = (uint64_t*)AllocatePages(1);
            uint64_t new_table_physical = (uint64_t)GetPhysical((uint64_t)new_table_virtual);

            // Copy kernel mapping over
            new_table_virtual[511] = kernel_mappings;
            // Add recursive mapping
            new_table_virtual[0x1F0] = new_table_physical | 0b11;
            // release(&mutex);
            return new_table_physical;
        }


        // Maps addresses in the current page table using recursive mapping.
        void Manager::MapPage(unsigned long phys, unsigned long virt, unsigned long options) {
            // acquire(&mutex);
            int lvl4 = (virt >> 39) & 0b111111111;
            int lvl3 = (virt >> 30) & 0b111111111;
            int lvl2 = (virt >> 21) & 0b111111111;
            int lvl1 = (virt >> 12) & 0b111111111;
            // Check if the level 3 table exists
            uint64_t* lvl4_table = CalculateRecursiveLevel4(0);
            if(~(lvl4_table[lvl4]) & 1) {
                lvl4_table[lvl4] = (uint64_t)PM::Manager::the().AllocatePages() | 0b111;
                InvalidatePage((uint64_t)CalculateRecursiveLevel3(lvl4));
                memset((void*)((uint64_t)CalculateRecursiveLevel3(lvl4)), 0x00, 4096);
            }
            uint64_t* lvl3_table = CalculateRecursiveLevel3(lvl4);
            // Check if the level 2 table exists
            if(~(lvl3_table[lvl3]) & 1) {
                lvl3_table[lvl3] = (uint64_t)PM::Manager::the().AllocatePages() | 0b111;
                InvalidatePage((uint64_t)CalculateRecursiveLevel2(lvl3, lvl4));
                memset((void*)((uint64_t)CalculateRecursiveLevel2(lvl3, lvl4)), 0x00, 4096);
            }
            uint64_t* lvl2_table = CalculateRecursiveLevel2(lvl3, lvl4);
            // Check if the level 1 table exists
            if(~(lvl2_table[lvl2]) & 1) {
                lvl2_table[lvl2] = (uint64_t)PM::Manager::the().AllocatePages() | 0b111;
                InvalidatePage((uint64_t)CalculateRecursiveLevel1(lvl2, lvl3, lvl4));
                memset((void*)((uint64_t)CalculateRecursiveLevel1(lvl2, lvl3, lvl4)), 0x00, 4096);
            }
            uint64_t* lvl1_table = CalculateRecursiveLevel1(lvl2, lvl3, lvl4);
            if(!(options & 1)) { lvl1_table[lvl1] = 0; return; } // If the present bit is not set, then just set to null
            // Set the entry
            ASSERT((phys & 0x8000000000000000) == 0, "Physical address would set NX bit!");
            lvl1_table[lvl1] = (phys & 0xFFFFFFFFFF000) | options;
            InvalidatePage((virt & 0xFFFFFFFFFF000));
            // release(&mutex);
        }
        



        // Modified version of MapPage that uses stiavle2's identity mapping to create new pages tables.
        void Manager::MapPageEarly(unsigned long phys, unsigned long virt) {
            int lvl4 = (virt >> 39) & 0b111111111;
            int lvl3 = (virt >> 30) & 0b111111111;
            int lvl2 = (virt >> 21) & 0b111111111;
            int lvl1 = (virt >> 12) & 0b111111111;
            // Check if the page tables have been allocated
            uint64_t* lvl3_table = (uint64_t*)page_table[lvl4];
            if(~((uint64_t)lvl3_table) & 1) {
                // Not present, make it and set its stuff
                page_table[lvl4] = (uint64_t)PM::Manager::the().AllocatePages();
                memset((void*)(page_table[lvl4]), 0x00, 512 * 8);
                lvl3_table = (uint64_t*)page_table[lvl4];
                page_table[lvl4] |= 0b11; // Present, writeable
            }
            lvl3_table = (uint64_t*)((uint64_t)lvl3_table & 0xFFFFFFFFFFFFF000);
            uint64_t* lvl2_table = (uint64_t*)(lvl3_table[lvl3]);
            if(~((uint64_t)lvl2_table) & 1) {
                // Not present, make it and set its stuff
                lvl3_table[lvl3] = (uint64_t)PM::Manager::the().AllocatePages();
                memset((void*)(lvl3_table[lvl3]), 0x00, 512 * 8);
                lvl2_table = (uint64_t*)(lvl3_table[lvl3]);
                lvl3_table[lvl3] |= 0b11; // Present, writeable    
            }
            lvl2_table = (uint64_t*)((uint64_t)lvl2_table & 0xFFFFFFFFFFFFF000);
            uint64_t* lvl1_table = (uint64_t*)(lvl2_table[lvl2]);
            if(~((uint64_t)lvl1_table) & 1) {
                // Not present, make it and set its stuff
                lvl2_table[lvl2] = (uint64_t)PM::Manager::the().AllocatePages();
                memset((void*)(lvl2_table[lvl2]), 0x00, 512 * 8);
                lvl1_table = (uint64_t*)(lvl2_table[lvl2]);
                lvl2_table[lvl2] |= 0b11; // Present, writeable
            }
            lvl1_table = (uint64_t*)((uint64_t)lvl1_table & 0xFFFFFFFFFFFFF000);
            lvl1_table[lvl1] = (phys & 0xFFFFFFFFFF000) | ((0b11) | (1 << 8)); // Set global bit
        }
    }
}
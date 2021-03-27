#include <mem.h>
#include <mem/virtmem.h>
#include <mem/physalloc.h>


namespace Kernel {
    // Return the addresses of these tables
    uint64_t* VirtualMemory::CalculateRecursiveLevel1(unsigned long lvl2, unsigned long lvl3, unsigned long lvl4, unsigned long offset) {
        return (uint64_t*)(0xFFFFF80000000000 | ((lvl4 & 0x1FF) << 30) | ((lvl3 & 0x1FF) << 21) | ((lvl2 & 0x1FF) << 12) | (offset & 0x1FFF));
    }

    uint64_t* VirtualMemory::CalculateRecursiveLevel2(unsigned long lvl3, unsigned long lvl4, unsigned long offset) {
        return (uint64_t*)(0xFFFFF87C00000000 | (lvl4 << 21) | (lvl3 << 12) | (offset & 0x1FFF));
    }

    uint64_t* VirtualMemory::CalculateRecursiveLevel3(unsigned long lvl4, unsigned long offset) {
        return (uint64_t*)(0xFFFFF87C3E000000 | (lvl4 << 12) | (offset & 0x1FFF));
    }

    uint64_t* VirtualMemory::CalculateRecursiveLevel4(unsigned long offset) {
        // This one is simple
        return (uint64_t*)(0xFFFFF87C3E1F0000 | (offset & 0x1FFF));
    }

    void VirtualMemory::TagLevel3Global(unsigned long entry) {
        // Check if the level 3 table exists
        uint64_t* lvl4_table = VirtualMemory::CalculateRecursiveLevel4(0);
        if(~(lvl4_table[entry]) & 1) { return;}
        lvl4_table[entry] |= (1 << 8);
    }

    void VirtualMemory::InvalidatePage(uint64_t address) {
        asm volatile("invlpg (%0)" : : "b"(address) : "memory");
        //SwitchPageTables(); // lol
        (void)address;
    }

    // Get the physical address of a mapping. Returns NULL if failed.
    uint64_t VirtualMemory::GetPhysical(uint64_t virt) {
        int lvl4 = (virt >> 39) & 0b111111111;
        int lvl3 = (virt >> 30) & 0b111111111;
        int lvl2 = (virt >> 21) & 0b111111111;
        int lvl1 = (virt >> 12) & 0b111111111;
        // Check if the level 3 table exists
        uint64_t* lvl4_table = VirtualMemory::CalculateRecursiveLevel4(0);
        if(~(lvl4_table[lvl4]) & 1) { return (uint64_t)NULL; }
        uint64_t* lvl3_table = VirtualMemory::CalculateRecursiveLevel3(lvl4);
        // Check if the level 2 table exists
        if(~(lvl3_table[lvl3]) & 1) { return (uint64_t)NULL; }
        uint64_t* lvl2_table = VirtualMemory::CalculateRecursiveLevel2(lvl3, lvl4);
        // Check if the level 1 table exists
        if(~(lvl2_table[lvl2]) & 1) { return (uint64_t)NULL; }
        uint64_t* lvl1_table = VirtualMemory::CalculateRecursiveLevel1(lvl2, lvl3, lvl4);
        // Check the entry
        uint64_t entry = lvl1_table[lvl1];
        if(~(entry) & 1) { return (uint64_t)NULL; }
        return entry & 0xFFFFFFFFFF000;
    } 

    void VirtualMemory::Init() {
        // We are still using stivale2's memory mapping, so we can just write to the pages allocated by the physical memory allocator.
        // We want to change as quickly as possible, so we just create the basic kernel mappings and go.
        // This will however be done by the _start function.
        page_table = (uint64_t*)__physmem_allocate_page();
        memset((void*)page_table, 0x00, 512 * 8);

        // Setup recursive mapping
        // stivale2 maps the kernel into a location requiring the last page,
        // so we cant really map it there.
        // We set the 0x1F0 entry to itself instead.
        page_table[0x1F0] = (uint64_t)page_table | 0b11; // Present, writeable

        // Calculate the bitmap size
        size_t kernel_page_bitmap_size = ((kernel_page_end - kernel_page_begin) / (4 * 1024)) / 8;
        kernel_page_bitmap = new uint8_t[kernel_page_bitmap_size];
    }

    void VirtualMemory::SwitchPageTables() {
        // Change CR3 register to page_table
        // This should auto-flush the TLB
        asm volatile("mov %0, %%cr3" : :"r"(page_table));
    }

    // Set the page table to a specific table.
    void VirtualMemory::SwitchPageTables(uint64_t table) {
        asm volatile("mov %0, %%cr3" : : "r"(table));
    }

    void* VirtualMemory::AllocatePages(size_t count) {
        // Go through the bitmap
        size_t total_page_count = ((kernel_page_end - kernel_page_begin) / (4 * 1024)) / 8;
        for(size_t page = 0; page < total_page_count; page++) {
            // Calculate byte and bit position
            int byte = page / 8;
            int bit = page % 8;

            // Check the bit
            if(!((kernel_page_bitmap[byte] >> bit) & 1)) {
                // Check if there are enough consecutive pages available
                bool enough = true;
                int page_failed = 0;
                for(size_t check_page = page; check_page < (page + count); check_page++) {
                    // Really fancy shit
                    if((kernel_page_bitmap[check_page / 8] >> (check_page % 8)) & 1) { enough = false; page_failed = check_page; break; }
                }
                if(!enough) {
                    // Not enough consecutive pages available
                    page = page_failed + 1;
                    continue;
                }
                // Enough pages are available
                for(size_t page_set = page + 1; page_set < (page + count); page_set++) {
                    kernel_page_bitmap[page_set / 8] |= 1 << (page_set % 8);
                }
                // Map some pages into there
                for(size_t map_pages = page; map_pages < (page + count); map_pages++) {
                    uint64_t virt_adr = kernel_page_begin + (map_pages * 4096);
                    uint64_t phys_adr = (uint64_t)__physmem_allocate_page();
                    Kernel::VirtualMemory::MapPage(phys_adr, virt_adr);
                }
                // Calculate return address and return
                return (void*)(kernel_page_begin + (page * 4096));
            }
        }
        return NULL; // No memory available
    }

    // TODO
    void VirtualMemory::FreePages(void* adr, int count) {
        (void)adr;
        (void)count;
    }



    // Create a new page table.
    uint64_t VirtualMemory::CreateNewPageTable() {
        // Get entry for the kernel mappings
        uint64_t kernel_mappings = CalculateRecursiveLevel4()[511];
        uint64_t* new_table_virtual = (uint64_t*)AllocatePages(1);
        uint64_t new_table_physical = (uint64_t)GetPhysical((uint64_t)new_table_virtual);

        // Copy kernel mapping over
        new_table_virtual[511] = kernel_mappings;
        return new_table_physical;
    }


    // Maps addresses in the current page table using recursive mapping.
    void VirtualMemory::MapPage(unsigned long phys, unsigned long virt, unsigned long options) {
        int lvl4 = (virt >> 39) & 0b111111111;
        int lvl3 = (virt >> 30) & 0b111111111;
        int lvl2 = (virt >> 21) & 0b111111111;
        int lvl1 = (virt >> 12) & 0b111111111;
        // Check if the level 3 table exists
        uint64_t* lvl4_table = CalculateRecursiveLevel4(0);
        if(~(lvl4_table[lvl4]) & 1) {
            lvl4_table[lvl4] = (uint64_t)__physmem_allocate_page() | 0b11;
            InvalidatePage((uint64_t)CalculateRecursiveLevel3(lvl4));
            memset((void*)((uint64_t)CalculateRecursiveLevel3(lvl4)), 0x00, 4096);
        }
        uint64_t* lvl3_table = CalculateRecursiveLevel3(lvl4);
        // Check if the level 2 table exists
        if(~(lvl3_table[lvl3]) & 1) {
            lvl3_table[lvl3] = (uint64_t)__physmem_allocate_page() | 0b11;
            InvalidatePage((uint64_t)CalculateRecursiveLevel2(lvl3, lvl4));
            memset((void*)((uint64_t)CalculateRecursiveLevel2(lvl3, lvl4)), 0x00, 4096);
        }
        uint64_t* lvl2_table = CalculateRecursiveLevel2(lvl3, lvl4);
        // Check if the level 1 table exists
        if(~(lvl2_table[lvl2]) & 1) {
            lvl2_table[lvl2] = (uint64_t)__physmem_allocate_page() | 0b11;
            InvalidatePage((uint64_t)CalculateRecursiveLevel1(lvl2, lvl3, lvl4));
            memset((void*)((uint64_t)CalculateRecursiveLevel1(lvl2, lvl3, lvl4)), 0x00, 4096);
        }
        uint64_t* lvl1_table = CalculateRecursiveLevel1(lvl2, lvl3, lvl4);
        // Set the entry
        lvl1_table[lvl1] = (phys & 0xFFFFFFFFFF000) | options;
        InvalidatePage((virt & 0xFFFFFFFFFF000));
    }
    



    // Modified version of MapPage that uses stiavle2's identity mapping to create new pages tables.
    void VirtualMemory::MapPageEarly(unsigned long phys, unsigned long virt) {
        int lvl4 = (virt >> 39) & 0b111111111;
        int lvl3 = (virt >> 30) & 0b111111111;
        int lvl2 = (virt >> 21) & 0b111111111;
        int lvl1 = (virt >> 12) & 0b111111111;
        // Check if the page tables have been allocated
        uint64_t* lvl3_table = (uint64_t*)page_table[lvl4];
        if(~((uint64_t)lvl3_table) & 1) {
            // Not present, make it and set its stuff
            page_table[lvl4] = (uint64_t)__physmem_allocate_page();
            memset((void*)(page_table[lvl4]), 0x00, 512 * 8);
            lvl3_table = (uint64_t*)page_table[lvl4];
            page_table[lvl4] |= 0b11; // Present, writeable
        }
        lvl3_table = (uint64_t*)((uint64_t)lvl3_table & 0xFFFFFFFFFFFFF000);
        uint64_t* lvl2_table = (uint64_t*)(lvl3_table[lvl3]);
        if(~((uint64_t)lvl2_table) & 1) {
            // Not present, make it and set its stuff
            lvl3_table[lvl3] = (uint64_t)__physmem_allocate_page();
            memset((void*)(lvl3_table[lvl3]), 0x00, 512 * 8);
            lvl2_table = (uint64_t*)(lvl3_table[lvl3]);
            lvl3_table[lvl3] |= 0b11; // Present, writeable    
        }
        lvl2_table = (uint64_t*)((uint64_t)lvl2_table & 0xFFFFFFFFFFFFF000);
        uint64_t* lvl1_table = (uint64_t*)(lvl2_table[lvl2]);
        if(~((uint64_t)lvl1_table) & 1) {
            // Not present, make it and set its stuff
            lvl2_table[lvl2] = (uint64_t)__physmem_allocate_page();
            memset((void*)(lvl2_table[lvl2]), 0x00, 512 * 8);
            lvl1_table = (uint64_t*)(lvl2_table[lvl2]);
            lvl2_table[lvl2] |= 0b11; // Present, writeable
        }
        lvl1_table = (uint64_t*)((uint64_t)lvl1_table & 0xFFFFFFFFFFFFF000);
        lvl1_table[lvl1] = (phys & 0xFFFFFFFFFF000) | ((0b11) | (1 << 8)); // Set global bit
    }
}
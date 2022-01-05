#include <mem.h>
#include <mem/VM/virtmem.h>
#include <mem/PM/physalloc.h>
#include <panic.h>
#include <debug/klog.h>

namespace Kernel {

namespace VM {
    // Contains the physical address of the original page table.
    uint64_t* page_table;

    // Vector of all allocated memory
    Vector<VMObject*> mem;

    // Mutex
    mutex_t mutex;

    // Virtual offset
    uint64_t virtual_offset = 0;

    static void InvalidatePage(uint64_t address) {
        asm volatile("invlpg (%0)" : : "b"(address) : "memory");
    }

    void Init(stivale2_struct_tag_memmap* memmap, uint64_t hhdm_offset) {
        virtual_offset = hhdm_offset;
        // Create the new kernel page table
        page_table = (uint64_t*)PM::AllocatePages();
        // Preallocate all lvl4 entries from 256-511
        for(size_t i = 256; i < 512; i++) {
            page_table[i] = PM::AllocatePages() | 0b11;
            memset((uint64_t*)(page_table[i] & ~(0b11)), 0, 4096);
        }
        // Allocate all memory entries in memmap
        for(size_t i = 0; i < memmap->entries; i++) {
            for(uint64_t curr = memmap->memmap[i].base; curr < (memmap->memmap[i].base + memmap->memmap[i].length); curr += 4096) {
                MapPage(curr, curr + virtual_offset, 0b11, page_table);
            }
            if(memmap->memmap[i].type == STIVALE2_MMAP_KERNEL_AND_MODULES) {
                // The kernel is also mapped to offset 0xffffffff80000000
                for(uint64_t curr = memmap->memmap[i].base; curr < (memmap->memmap[i].base + memmap->memmap[i].length); curr += 4096) {
                    MapPage(curr, curr + 0xffffffff80000000, 0b11, page_table);
                }
            }
        }
        // Change to this page table
        SwitchPageTables((uint64_t)page_table);
    }



    uint64_t CurrentPageTable() {
        uint64_t ret;
        asm volatile("mov %%cr3, %0" : "=r"(ret));
        return ret;
    }

    // Get the physical address of a mapping. Returns NULL if failed.
    uint64_t GetPhysical(uint64_t virt) {
        int lvl4 = (virt >> 39) & 0b111111111;
        int lvl3 = (virt >> 30) & 0b111111111;
        int lvl2 = (virt >> 21) & 0b111111111;
        int lvl1 = (virt >> 12) & 0b111111111;
        // Check if the level 3 table exists
        uint64_t* lvl4_table = (uint64_t*)(CurrentPageTable() + virtual_offset);
        if(~(lvl4_table[lvl4]) & 1) { return (uint64_t)NULL; }
        uint64_t* lvl3_table = (uint64_t*)((lvl4_table[lvl4] & 0xffffffffff000) + virtual_offset);
        // Check if the level 2 table exists
        if(~(lvl3_table[lvl3]) & 1) { return (uint64_t)NULL; }
        uint64_t* lvl2_table = (uint64_t*)((lvl3_table[lvl3] & 0xffffffffff000) + virtual_offset);
        // Check if the level 1 table exists
        if(~(lvl2_table[lvl2]) & 1) { return (uint64_t)NULL; }
        uint64_t* lvl1_table = (uint64_t*)((lvl2_table[lvl2] & 0xffffffffff000) + virtual_offset);
        // Check the entry
        uint64_t entry = lvl1_table[lvl1];
        if(~(entry) & 1) { return (uint64_t)NULL; }
        return entry & 0xFFFFFFFFFF000;
    } 

    uint64_t GetVirtualOffset() { return virtual_offset; }

    // Create a new page table.
    uint64_t CreateNewPageTable() {
        uint64_t new_table_physical = PM::AllocatePages();
        // Copy the top entry
        uint64_t* table_virt = (uint64_t*)(new_table_physical + virtual_offset);
        for(size_t i = 256; i < 512; i++) {
            table_virt[i] = ((uint64_t*)((uint64_t)page_table + virtual_offset))[i];
        }
        return new_table_physical;
    }


    // Maps addresses in the current page table.
    void MapPage(unsigned long phys, unsigned long virt, unsigned long options) {
        //acquire(&mutex);
        int lvl4 = (virt >> 39) & 0b111111111;
        int lvl3 = (virt >> 30) & 0b111111111;
        int lvl2 = (virt >> 21) & 0b111111111;
        int lvl1 = (virt >> 12) & 0b111111111;
        // Check if the level 3 table exists
        uint64_t* lvl4_table = (uint64_t*)(CurrentPageTable() + virtual_offset);
        if(~(lvl4_table[lvl4]) & 1) {
            lvl4_table[lvl4] = (uint64_t)PM::AllocatePages() | 0b111;
            InvalidatePage(lvl4_table[lvl4] + virtual_offset);
            memset((void*)((lvl4_table[lvl4] & 0xffffffffff000) + virtual_offset), 0x00, 4096);
        }
        uint64_t* lvl3_table = (uint64_t*)((lvl4_table[lvl4] & 0xffffffffff000) + virtual_offset);
        // Check if the level 2 table exists
        if(~(lvl3_table[lvl3]) & 1) {
            lvl3_table[lvl3] = (uint64_t)PM::AllocatePages() | 0b111;
            InvalidatePage(lvl3_table[lvl3] + virtual_offset);
            memset((void*)((lvl3_table[lvl3] & 0xffffffffff000) + virtual_offset), 0x00, 4096);
        }
        uint64_t* lvl2_table = (uint64_t*)((lvl3_table[lvl3] & 0xffffffffff000) + virtual_offset);
        // Check if the level 1 table exists
        if(~(lvl2_table[lvl2]) & 1) {
            lvl2_table[lvl2] = (uint64_t)PM::AllocatePages() | 0b111;
            InvalidatePage(lvl2_table[lvl2] + virtual_offset);
            memset((void*)((lvl2_table[lvl2] & 0xffffffffff000) + virtual_offset), 0x00, 4096);
        }
        uint64_t* lvl1_table = (uint64_t*)((lvl2_table[lvl2] & 0xffffffffff000) + virtual_offset);
        if(!(options & 1)) { lvl1_table[lvl1] = 0; return; } // If the present bit is not set, then just set to null
        // Set the entry
        ASSERT((phys & 0x8000000000000000) == 0, "Physical address would set NX bit!");
        lvl1_table[lvl1] = (phys & 0xFFFFFFFFFF000) | options;
        InvalidatePage((virt & 0xFFFFFFFFFF000));
        //release(&mutex);
    }

    // Map pages in a specific page table
    // Table can be either a physical or a virtual address
    void MapPage(unsigned long phys, unsigned long virt, unsigned long options, uint64_t* table) {
        //acquire(&mutex);
        int lvl4 = (virt >> 39) & 0b111111111;
        int lvl3 = (virt >> 30) & 0b111111111;
        int lvl2 = (virt >> 21) & 0b111111111;
        int lvl1 = (virt >> 12) & 0b111111111;
        // Check if the level 3 table exists
        uint64_t* lvl4_table;
        if((uint64_t)table & (1ULL << 63)) {
            lvl4_table = table;
        } else {
            lvl4_table = (uint64_t*)((uint64_t)(table) + virtual_offset);
        }
        if(~(lvl4_table[lvl4]) & 1) {
            lvl4_table[lvl4] = (uint64_t)PM::AllocatePages() | 0b111;
            InvalidatePage(lvl4_table[lvl4] + virtual_offset);
            memset((void*)((lvl4_table[lvl4] & 0xffffffffff000) + virtual_offset), 0x00, 4096);
        }
        uint64_t* lvl3_table = (uint64_t*)((lvl4_table[lvl4] & 0xffffffffff000) + virtual_offset);
        // Check if the level 2 table exists
        if(~(lvl3_table[lvl3]) & 1) {
            lvl3_table[lvl3] = (uint64_t)PM::AllocatePages() | 0b111;
            InvalidatePage(lvl3_table[lvl3] + virtual_offset);
            memset((void*)((lvl3_table[lvl3] & 0xffffffffff000) + virtual_offset), 0x00, 4096);
        }
        uint64_t* lvl2_table = (uint64_t*)((lvl3_table[lvl3] & 0xffffffffff000) + virtual_offset);
        // Check if the level 1 table exists
        if(~(lvl2_table[lvl2]) & 1) {
            lvl2_table[lvl2] = (uint64_t)PM::AllocatePages() | 0b111;
            InvalidatePage(lvl2_table[lvl2] + virtual_offset);
            memset((void*)((lvl2_table[lvl2] & 0xffffffffff000) + virtual_offset), 0x00, 4096);
        }
        uint64_t* lvl1_table = (uint64_t*)((lvl2_table[lvl2] & 0xffffffffff000) + virtual_offset);
        if(!(options & 1)) { lvl1_table[lvl1] = 0; return; } // If the present bit is not set, then just set to null
        // Set the entry
        ASSERT((phys & 0x8000000000000000) == 0, "Physical address would set NX bit!");
        lvl1_table[lvl1] = (phys & 0xFFFFFFFFFF000) | options;
        InvalidatePage((virt & 0xFFFFFFFFFF000));
        //release(&mutex);
    }


    ///
    /// Memory allocation stuff
    ///

    // Start of kernel pages
    uint64_t kernel_page_begin = 0xffffc00000000000;
    const uint64_t kernel_page_end = 0xffffff0000000000;

    // Hint virtual address.
    uint64_t hint = kernel_page_begin;

    static void AllocateAndMap(uint64_t virt, size_t count) {
        for(size_t i = 0; i < count; i++) {
            uint64_t phys = PM::AllocatePages();
            if(!phys) { Debug::Panic("VM: kernel page allocation failed"); }
            MapPage(phys, virt + (i * 4096), 0b11);
        }
    }

    static uint64_t CheckAndAllocate(uint64_t start, uint64_t end, size_t count) {
        size_t amount_contigous = 0;
        for(size_t curr = start; start < end; start += 4096)  {
            if(GetPhysical(curr)) { amount_contigous = 0; continue; }
            amount_contigous++;
            if(amount_contigous >= count) {
                // We have enough
                #if defined(VM_LOG_ALLOC) && VM_LOG_ALLOC
                Kernel::Debug::SerialPrintf("[VM]: allocating %i pages at %x\n\r", count, curr);
                #endif
                AllocateAndMap(curr, count);
                return curr;
            }
        }
        return 0;
    }

    void* AllocatePages(size_t count) {
        acquire(&mutex);
        // Check the hint
        uint64_t curr = CheckAndAllocate(hint, kernel_page_end, count);
        if(curr) {
            hint = curr + (count * 4096);
            release(&mutex);
            return (void*)curr;
        }
        // Check before the hint
        curr = CheckAndAllocate(kernel_page_begin, hint, count);
        if(curr) {
            hint = curr + (count * 4096);
            release(&mutex);
            return (void*)curr;
        }
        Debug::Panic("VM: could not find space in the address space");
        release(&mutex);
        return NULL;
    }

    #if defined(VM_FREE) && VM_FREE
    void FreePages(void* adr, size_t pages) {
        #if defined(VM_LOG_FREE) && VM_LOG_FREE
        Kernel::Debug::SerialPrintf("[VM]: deallocating %i pages at %x\n\r", pages, (uint64_t)adr);
        #endif
        acquire(&mutex);
        // Loop through each page and see if we can find this addr
        for(uint64_t curr = (uint64_t)adr; curr < ((uint64_t)adr + (pages * 4096)); curr += 4096) {
            uint64_t phys = GetPhysical(curr);
            if(!phys) {
                KLog::the().printf("VM: FreePages() got area with no physical mapping\n\r");
            } else {
                PM::FreePages(phys);
            }
            MapPage(0xDEADA550000, curr, 0);
        }
        release(&mutex); 
    }
    #else
    void FreePages(void*adr) {
        (void)adr;
    }
    #endif

}

}
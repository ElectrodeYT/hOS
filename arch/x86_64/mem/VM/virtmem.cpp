#include <mem.h>
#include <mem/VM/virtmem.h>
#include <mem/PM/physalloc.h>
#include <panic.h>

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

    // One entry in the vmm_entry_page
    struct vmm_entry {
        uint64_t base;
        uint64_t len;
        uint8_t state;
    } __attribute__((packed));

    // A entry describing pages and their states.
    // One struct does many pages, as this struct is designed
    // to fit into one physical page
    // They are accessed by the virtual offset
    struct vmm_entry_page {
        vmm_entry_page* next;
        vmm_entry_page* prev;
        vmm_entry entries[240];
    };

    vmm_entry_page* pages;
    vmm_entry_page* hint;
    size_t hint_id;

    static void AllocateAndMap(uint64_t virt, size_t count) {
        for(size_t i = 0; i < count; i++) {
            uint64_t phys = PM::AllocatePages();
            if(!phys) { Debug::Panic("VM: kernel page allocation failed"); }
            MapPage(phys, virt + (i * 4096), 0b11);
        }
    }

    void* AllocatePages(size_t count) {
        acquire(&mutex);
        if(!pages) {
            // No pages have yet to be allocated, init the page list
            pages = (vmm_entry_page*)(PM::AllocatePages() + virtual_offset);
            memset(pages, 0, 4096);
            pages->entries[0].base = kernel_page_begin;
            pages->entries[0].len = (kernel_page_end - kernel_page_begin) / 4096;
            hint = pages;
            hint_id = 0;
        }
        // Check the hint firsst
        if(hint->entries[hint_id].base && hint->entries[hint_id].state == 0 && hint->entries[hint_id].len >= count) {
            // Hint is big enough, try to find the next slot that is empty
            for(size_t i = hint_id + 1; i < 240; i++) {
                if(hint->entries[i].base == 0) {
                    // Empty slot found, put the remaining pages in there
                    uint64_t old_hint_id = hint_id;
                    if(hint->entries[hint_id].len - count) {
                        hint->entries[i].base = hint->entries[hint_id].base + (count * 4096);
                        hint->entries[i].len = hint->entries[hint_id].len - count;
                        hint->entries[i].state = 0;
                        // Update hint pointer
                        hint_id = i;
                    } else {
                        // We have (somehow) used the entire memory space, reset hint to the first page
                        hint = pages;
                        hint_id = 0;
                    }
                    // Set current hint to used
                    hint->entries[old_hint_id].len = count;
                    hint->entries[old_hint_id].state = 1;

                    AllocateAndMap(hint->entries[old_hint_id].base, count);
                    release(&mutex);
                    return (void*)(hint->entries[old_hint_id].base);
                }
            }
            // Didnt find a empty entry here, see if there is a page afterwards
            vmm_entry_page* curr_page = hint;
            while(curr_page->next) {
                curr_page = curr_page->next;
                for(size_t i = 0; i < 240; i++) {
                    if(hint->entries[i].base == 0) {
                        // Empty slot found, put the remaining pages in there
                        uint64_t old_hint_id = hint_id;
                        if(hint->entries[hint_id].len - count) {
                            hint->entries[i].base = hint->entries[hint_id].base + (count * 4096);
                            hint->entries[i].len = hint->entries[hint_id].len - count;
                            hint->entries[i].state = 0;
                            // Update hint to this page
                            hint = curr_page;
                            hint_id = i;
                        } else {
                            // We have (somehow) used the entire memory space, reset hint to the first page
                            hint = pages;
                            hint_id = 0;
                        }
                        // Set current hint to used
                        hint->entries[old_hint_id].len = count;
                        hint->entries[old_hint_id].state = 1;
                        AllocateAndMap(hint->entries[old_hint_id].base, count);

                        release(&mutex);
                        return (void*)(hint->entries[old_hint_id].base);
                    }
                }
            }
            // Still havent found a empty slot, create a new page for this
            curr_page->next = (vmm_entry_page*)(PM::AllocatePages() + virtual_offset);
            memset(curr_page->next, 0, 4096);
            uint64_t old_hint_id = hint_id;
            if(hint->entries[hint_id].len - count) {
                curr_page->next->entries[0].base = hint->entries[hint_id].base + (count * 4096);
                curr_page->next->entries[0].len = hint->entries[hint_id].len - count;
                curr_page->next->entries[0].state = 0;
                // Update hint to this page
                hint = curr_page->next;
                hint_id = 0;
            } else {
                // We have (somehow) used the entire memory space, reset hint to the first page
                hint = pages;
                hint_id = 0;
            }
            // Set current hint to used
            hint->entries[old_hint_id].len = count;
            hint->entries[old_hint_id].state = 1;
            AllocateAndMap(hint->entries[old_hint_id].base, count);

            release(&mutex);
            return (void*)(hint->entries[old_hint_id].base);
        }
        // TODO: implement hint failure
        // Shouldnt really happen, we have so much virtual memory area available
        Debug::Panic("VM: todo: implement hint fail in AllocatePages()");
        release(&mutex);
        return NULL;
    }

    void FreePages(void* adr) {
        acquire(&mutex);
        // Loop through each page and see if we can find this addr
        uint64_t addr_int = (uint64_t)adr;
        vmm_entry_page* curr_pages = pages;
        while(curr_pages) {
            for(size_t i = 0; i < 240; i++) {
                if(curr_pages->entries[i].base == addr_int) {
                    // Free the pages through the PM
                    for(size_t y = 0; y < curr_pages->entries[i].len; y++) {
                        uint64_t phys = GetPhysical(curr_pages->entries[i].base + (y * 4096));
                        if(phys) { PM::FreePages(phys); }
                        MapPage(0, curr_pages->entries[i].base + (y * 4096), 0);
                    }
                    // Set these to free
                    curr_pages->entries[i].state = 0;
                    release(&mutex);
                    return;
                }
            }
        }
        // Couldnt find it, meh
        release(&mutex); 
    }


}

}
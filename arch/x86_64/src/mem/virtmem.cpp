#include <mem.h>
#include <virtmem.h>
#include <mem/physalloc.h>

// Contains the physical address of the page table.
uint64_t* page_table;

void __init_virtual_memory() {
    // We are still using stivale2's memory mapping, so we can just write to the pages allocated by the physical memory allocator.
    // We want to change as quickly as possible, so we just create the basic kernel mappings and go.
    // This will however be done by the _start function.
    page_table = (uint64_t*)__physmem_allocate_page();
    memset((void*)page_table, 0x00, 512 * 8);

    // Setup recursive mapping on the last page
    page_table[511] = (uint64_t)page_table | 0b11; // Present, writeable
}

void __virtmem_switch_page_tables() {

}

void* __virtmem_allocate_pages(int count) {

}

void __virtmem_free_pages(void* adr, int count) {

}

// Maps addresses.
void __virtmem_map(unsigned long phys, unsigned long virt) {

}
// Modified version of __virtmem_map that uses stiavle2's identity mapping to create new pages tables.
void __virtmem_early_map(unsigned long phys, unsigned long virt) {
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
        page_table[lvl4] |= 0b11; // Present, writeable
    }
    uint64_t* lvl2_table = (uint64_t*)lvl3_table[lvl3];
    if(~((uint64_t)lvl2_table) & 1) {
        // Not present, make it and set its stuff
        lvl3_table[lvl3] = (uint64_t)__physmem_allocate_page();
        memset((void*)(lvl3_table[lvl3]), 0x00, 512 * 8);
        lvl3_table[lvl3] |= 0b11; // Present, writeable
    }
    uint64_t* lvl1_table = (uint64_t*)lvl2_table[lvl2];
    if(~((uint64_t)lvl1_table) & 1) {
        // Not present, make it and set its stuff
        lvl2_table[lvl2] = (uint64_t)__physmem_allocate_page();
        memset((void*)(lvl2_table[lvl2]), 0x00, 512 * 8);
        lvl2_table[lvl2] |= 0b11; // Present, writeable
    }
    lvl1_table[lvl1] = (virt & 0xFFFFFFFFFF000) | ((0b11) | (1 << 8)); // Set global bit
}
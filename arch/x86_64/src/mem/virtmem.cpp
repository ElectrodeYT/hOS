#include <mem.h>
#include <mem/virtmem.h>
#include <mem/physalloc.h>

// Contains the physical address of the page table.
uint64_t* page_table;

// Define Kernel page area.
uint8_t* kernel_page_bitmap;
uint64_t kernel_page_begin = 0xffffffff90000000;
uint64_t kernel_page_end = 0xfffffffffffff000;

void __init_virtual_memory() {
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

void __virtmem_switch_page_tables() {
    // Change CR3 register to page_table
    // This should auto-flush the TLB
    asm volatile("mov %0, %%cr3" : :"r"(page_table));
}

void* __virtmem_allocate_pages(int count) {
    (void)count;
    return NULL;
}

void __virtmem_free_pages(void* adr, int count) {
    (void)adr;
    (void)count;
}

// Maps addresses.
void __virtmem_map(unsigned long phys, unsigned long virt) {
    (void)phys;
    (void)virt;
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
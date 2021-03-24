#ifndef VIRTMEM_H
#define VIRTMEM_H

// Initialize virtual memory.
// Also creates the page tables, and changes to them.
void __init_virtual_memory();

void __virtmem_switch_page_tables();

void* __virtmem_allocate_pages(int count);
void __virtmem_free_pages(void* adr, int count);

// Maps addresses.
void __virtmem_map(unsigned long phys, unsigned long virt);
// Modified version of __virtmem_map that uses stiavle2's identity mapping to create new pages tables.
void __virtmem_early_map(unsigned long phys, unsigned long virt);


#endif
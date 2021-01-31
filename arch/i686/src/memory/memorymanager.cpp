#include <memory/memorymanager.h>
#include <memory/heap.h>
#include <multiboot/multiboot.h>
#include <debug/debug_print.h>

extern "C" const uint32_t kernel_begin;
extern "C" const uint32_t kernel_end;




namespace Kernel {
    namespace MemoryManager {
        namespace PageFrameAllocator {

            static uint32_t biggest_adr = 0;
            static uint32_t biggest_len = 0;

            static uint32_t* page_bitmap = 0;
            static uint32_t page_bitmap_size = 0;



            int InitPageFrameAllocator(void* multiboot_info) {
                // We want to loop through the mmap
                volatile multiboot_info_t* multiboot = (volatile multiboot_info_t*)multiboot_info;
                // Halt if the multiboot info does not have memory map info
                if(!(multiboot->flags & MULTIBOOT_INFO_MEM_MAP)) { debug_puts("Multiboot info does not have memory map! Halting.\n\r"); while(1); }
                for(uint32_t i = 0; i < multiboot->mmap_length; i += sizeof(multiboot_memory_map_t)) {
                    // Get entry
                    // mmap_adr is a virtual address, so we need to add the kernel virtual base to it.
                    multiboot_memory_map_t* entry = (multiboot_memory_map_t*)(multiboot->mmap_addr + KernelVirtualBase + i);
                    // Check if we can use this segment
                    if(entry->addr >> 32) { debug_puts("Skipping entry "); debug_puti(i); debug_puts(" in the mmap table, as it has a PAE address.\n\r"); continue; }
                    // Check if its a null entry (sometimes happens)
                    if(entry->addr == 0 && entry->len == 0) { debug_puts("Skipping Null entry"); continue; }
                    // Check if its available conventinal memory
                    if(entry->type != MULTIBOOT_MEMORY_AVAILABLE) { continue; }
                    // Check if its bigger then the one we got right now
                    if(entry->len > biggest_len) {
                        // Set it as the newest free memory
                        biggest_adr = entry->addr;
                        biggest_len = entry->len;
                    }
                }
                // Check if anything got allocated
                if(biggest_len == 0) { while(1); }

                // We have now got the biggest free entry available
                debug_puts("Biggest entry: "); debug_puti(biggest_adr, 16); debug_put(' '); debug_puti(biggest_len, 16); debug_puts("\n\r");

                uint32_t kernel_begin_address = (uint32_t)(&kernel_begin) - KernelVirtualBase;
                uint32_t kernel_end_address = (uint32_t)(&kernel_end) - KernelVirtualBase;
                // It's likely that we are using the same segment of memory the kernel occupies, so we need to check if the kernel is included
                if(kernel_begin_address >= biggest_adr || kernel_end_address >= biggest_adr) {
                    // The kernel occupies some of the memory, so we set the begining of the segment to the first page without any kernel in it
                    uint32_t new_biggest_adr = (kernel_end_address & ~(0x1FFF));
                    if(kernel_end_address & 0x1FFF) { new_biggest_adr += 0x1000; } // Make sure its the first page after the kernel
                    // Update biggest_len
                    biggest_len -= biggest_adr - new_biggest_adr;
                    biggest_adr = new_biggest_adr;
                }

                // Create the page bitmap
                page_bitmap_size = (biggest_len / (4 * 1024)) / 32; // 32 bits in one uint32_t
                page_bitmap = new uint32_t[page_bitmap_size];

                for(uint32_t i = 0; i < page_bitmap_size; i++) { page_bitmap[i] = 0; }

                debug_puts("Initialized page allocator. Phys page begin: "); debug_puti(biggest_adr, 16); debug_puts(" size: "); debug_puti(biggest_len, 16); debug_puts(" bitmap begin: "); debug_puti((uint32_t)page_bitmap, 16); debug_puts("\n\r");
                return 0;
            }

            void* AllocatePage() {
                // Loop through all the ints
                for(uint32_t i = 0; i < page_bitmap_size; i++) {
                    if(page_bitmap[i] != 0xFFFFFFFF) {
                        // There is a free page here
                        for(int x = 0; x < 32; x++) {
                            if(!(page_bitmap[i] & (1 << x))) {
                                // Got a free page
                                int page_id = i + x;
                                // Set the bit
                                page_bitmap[i] |= (1 << x);
                                return (void*)(biggest_adr + (page_id * (4 * 1024)));
                            }
                        }
                        // TODO: ASSERT
                    }
                }
                return NULL;
            }

            int FreePage(void* page) {
                // Get Page ID
                int page_id = ((uint32_t)(page) - biggest_adr) / (4 * 1024);
                // Unset the bit it corresponds to
                page_bitmap[page_id / 32] &= ~(1 << (page_id % 32));
                return 0;
            }
        }

        namespace VirtualMemory {
            // Page directory
            uint32_t* PageDirectory;

            // This function maps a 4K page.
            // It will create a page table if one does not exist.
            int MapPage(uint32_t phys, uint32_t virt) {
                // Check the page directory
                uint32_t* pd = (uint32_t*)0xFFFFF000;
                uint32_t* page_table = (uint32_t*)(0xFFC00000 + (0x1000 * (virt >> 22)));
                // Check if the page table exists
                if(!(pd[virt >> 22] & 1)) {
                    // It doesnt, create it
                    pd[virt >> 22] = (uint32_t)PageFrameAllocator::AllocatePage() | 0b11;
                    // Flush the tlb entry
                    asm volatile("invlpg (%0)" : : "r" (virt) : "memory");
                    // Clear it
                    for(int i = 0; i < 1024; i++) {
                        page_table[i] = 0;
                    }
                }
                // The page table exists / was created
                page_table[(virt >> 12) & 0x3FFF] = phys | 0b11;
                return 0;
            }

            int CheckIfPageIsMapped(uint32_t adr) {
                // Check the page directory
                uint32_t* pd_offset = (uint32_t*)(0xFFFFF000 + ((adr >> 22) * 4));
                if(!(*pd_offset & 1)) { return 0; } // Page table doesnt exist
                // Check if its a region (4MiB) mapping
                if(*pd_offset & 0b10000000) { return 1; }
                // Check the page table
                uint32_t* page_table = (uint32_t*)(0xFFC00000 + (0x1000 * (adr >> 22))); // page table this mapping is in
                return page_table[(adr >> 12) & 0x3FFF] & 1; // Return if the first bit in the page table is set
            }

            int InitVM() {
                // The page frame allocator should be setup now
                // We will use kmalloc() directly to allocate the page directory
                // For the actual page tables we want to allocate physical pages
                PageDirectory = (uint32_t*)kmalloc_imp(1024 * 32, 0x1FFFFF); // Page aligned
                if(PageDirectory == nullptr) {
                    debug_puts("Could not allocate page directory!");
                    while(1);
                }
                // Zero that shit
                for(int i = 0; i < 1024; i++) {
                    PageDirectory[i] = 0;
                }

                // Setup the recursive mapping
                uint32_t pd_physical = (uint32_t)PageDirectory - KernelVirtualBase;
                PageDirectory[1023] = pd_physical | 0b11;

                // Map the kernel
                uint32_t kernel_begin_address = (uint32_t)(&kernel_begin) - KernelVirtualBase;
                uint32_t kernel_end_address = (uint32_t)(&kernel_end) - KernelVirtualBase;
                
                for(uint32_t curr_adr = kernel_begin_address; curr_adr <= kernel_end_address; curr_adr += 4 * 1024 * 1024) {
                    // We map the kernel with PD Regions
                    uint32_t virtual_adr = curr_adr + KernelVirtualBase;
                    PageDirectory[virtual_adr >> 22] = curr_adr & ~(0x3FFFFF) | 0b10000001;
                }

                asm volatile("mov %0, %%cr3;" : : "a" (pd_physical));
                return 0;
            }

            // We begin the kernel virtual allocated pages here.
            static uint32_t check_page_begin = 0xD0000000;
            // We stop allocations here.
            static uint32_t max_allocated_pages = 0xE0000000;
            void* GetPage() {
                while(check_page_begin < max_allocated_pages) {
                    // Check page
                    uint32_t virt_page = check_page_begin;
                    check_page_begin += 4 * 1024;
                    if(CheckIfPageIsMapped(check_page_begin)) {
                        // This page is mapped,  and continue
                        continue;
                    }
                    // Allocate physical page
                    uint32_t physical_page = (uint32_t)PageFrameAllocator::AllocatePage();
                    if(physical_page == NULL) { asm volatile("cli; hlt"); }
                    // Now map that page
                    MapPage(physical_page, virt_page);
                    return (void*)virt_page;
                }
                asm volatile("cli; hlt");
                return NULL; // will never happen
            }
        }
    }
}
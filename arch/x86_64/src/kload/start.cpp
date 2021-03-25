#include <stdint.h>
#include <stddef.h>
#include <stivale2.h>
#include <early-boot.h>
#include <mem.h>
#include <mem/physalloc.h>
#include <mem/virtmem.h>
#include <debug/serial.h>
#include <kmain.h>

typedef void (*ctor_constructor)();
extern "C" ctor_constructor start_ctors;
extern "C" ctor_constructor end_ctors;

// The following will be our kernel's entry point.
extern "C" void __attribute__((optimize("O0"))) _start(struct stivale2_struct *stivale2_struct) {
    // We are in a very limited enviroment right now, including the fact that the kernel heap doesnt work yet
    // (on purpose, to prevent undef. behaviour incase we accidentally call it)

    // Very first thing is we create the gdt  
    load_gdt();
    // We initialize the kernel internal heap as well, it is needed for initializing 
    __init_heap();

    // Get stivale memory map tag
    stivale2_struct_tag_memmap* memmap_tag = (stivale2_struct_tag_memmap*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);

    // Save the entry which has the kernel in it
    stivale2_mmap_entry* kernel_mapping = NULL;

    // Check that the memory map is availabe
    if(memmap_tag->entries == 0) { for(;;); } // No output is setup yet, just hang
    physmem_ll* physical_mem_ll = new physmem_ll;
    physical_mem_ll->base = 0;
    physical_mem_ll->next = 0;
    physical_mem_ll->size = 0;
    physical_mem_ll->type = 0;
    physmem_ll* curr_phys_mem_ll = physical_mem_ll;
    for(unsigned int i = 0; i < memmap_tag->entries; i++) {
        uint8_t entry_type = 255;
        // Convert stivale2 type to kernel internal type
        switch(memmap_tag->memmap[i].type) {
            case STIVALE2_MMAP_USABLE: entry_type = PHYSMEM_LL_TYPE_USEABLE; break;
            case STIVALE2_MMAP_RESERVED: entry_type = PHYSMEM_LL_TYPE_RESERVED; break;
            case STIVALE2_MMAP_ACPI_RECLAIMABLE: entry_type = PHYSMEM_LL_TYPE_ACPI_REC; break;
            case STIVALE2_MMAP_ACPI_NVS: entry_type = PHYSMEM_LL_TYPE_ACPI_NVS; break;
            case STIVALE2_MMAP_BAD_MEMORY: entry_type = PHYSMEM_LL_TYPE_BAD; break;
            case STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE: entry_type = PHYSMEM_LL_TYPE_BOOTLOADER; break;
            case STIVALE2_MMAP_KERNEL_AND_MODULES: entry_type = PHYSMEM_LL_TYPE_KERNEL; break;
            default: break; // Leave it with the error entry
        }

        // Perform check if the memory area is in the low area
        if(entry_type == PHYSMEM_LL_TYPE_USEABLE && memmap_tag->memmap[i].base < 0x1000) {
            entry_type = PHYSMEM_LL_TYPE_LOWMEM;
        }

        curr_phys_mem_ll->base = memmap_tag->memmap[i].base;
        curr_phys_mem_ll->type = entry_type;
        curr_phys_mem_ll->size = memmap_tag->memmap[i].length;
        
        // If this isnt the last one, allocate next one and change to it
        if(i != (memmap_tag->entries - 1)) {
            curr_phys_mem_ll->next = new physmem_ll;
            curr_phys_mem_ll = curr_phys_mem_ll->next;
        }

        // If this is the kernel mapping, save it
        if(memmap_tag->memmap[i].type == STIVALE2_MMAP_KERNEL_AND_MODULES) {
            kernel_mapping = &(memmap_tag->memmap[i]);
        }
    }
    // Now initialize the physical memory allocator
    __init_physical_allocator(physical_mem_ll);

    // Initialize virtual memory
    __init_virtual_memory();

    // Call the global constructors
    for (ctor_constructor* ctor = &start_ctors; ctor < &end_ctors; ctor++) {
        (*ctor)();
    }

    // Map kernel
    uint64_t kernel_phys_begin = (kernel_mapping->base & 0xFFFFFFFFFF000);
    uint64_t kernel_phys_end = ((kernel_mapping->base + kernel_mapping->length) & 0xFFFFFFFFFF000);

    uint64_t kernel_virt_begin = 0xffffffff80200000;

    // Some logic incase the kernel overflows the last page
    if((kernel_mapping->base + kernel_mapping->length) & 0xFFF) { kernel_phys_end += 4 * 1024; }

    for(uint64_t current_map = kernel_phys_begin; current_map < kernel_phys_end; current_map += 4 * 1024) {
        __virtmem_early_map(current_map, kernel_virt_begin);
        kernel_virt_begin += 4 * 1024;
    }

    // We can now switch to the new page table
    __virtmem_switch_page_tables();

    // We are in a much safer place now; dereferencing null pointers will for example now crash, before it would have been identity mapped to physical memory.
    // We can also setup simple debug output, and initialize the panic function (errors previously would have simply crashed the system)

    // Setup serial debug output
    __init_serial();

    // Basic init has occured, we can call the main now
    Kernel::KernelMain();
}
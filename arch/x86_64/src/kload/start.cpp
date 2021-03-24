#include <stdint.h>
#include <stddef.h>
#include <stivale2.h>
#include <early-boot.h>
#include <mem.h>
#include <mem/physalloc.h>

// The following will be our kernel's entry point.
extern "C" void _start(struct stivale2_struct *stivale2_struct) {
    // We are in a very limited enviroment right now, including the fact that the kernel heap doesnt work yet
    // (on purpose, to prevent undef. behaviour incase we accidentally call it)

    // Very first thing is we create the gdt  
    load_gdt();
    // We initialize the kernel internal heap as well, it is needed for initializing 
    __init_heap();

    // Get stivale memory map tag
    stivale2_struct_tag_memmap* memmap_tag = (stivale2_struct_tag_memmap*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);

    // Check that the memory map is availabe
    if(memmap_tag->entries == 0) { for(;;); } // No output is setup yet, just hang
    physmem_ll* physical_mem_ll = new physmem_ll;
    physical_mem_ll->base = 0;
    physical_mem_ll->next = 0;
    physical_mem_ll->size = 0;
    physical_mem_ll->type = 0;
    physmem_ll* curr_phys_mem_ll = physical_mem_ll;
    for(int i = 0; i < memmap_tag->entries; i++) {
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
        curr_phys_mem_ll->base = memmap_tag->memmap[i].base;
        curr_phys_mem_ll->type = entry_type;
        curr_phys_mem_ll->size = memmap_tag->memmap[i].length;
        
        // If this isnt the last one, allocate next one and change to it
        if(i != (memmap_tag->entries - 1)) {
            curr_phys_mem_ll->next = new physmem_ll;
            curr_phys_mem_ll = curr_phys_mem_ll->next;
        }
    }
    // Now initialize the physical memory allocator
    __init_physical_allocator(physical_mem_ll);
    // We're done, just hang...
    for (;;) {
        asm ("hlt");
    }
}
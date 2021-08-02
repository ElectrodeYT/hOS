#include <stdint.h>
#include <stddef.h>
#include <stivale2.h>
#include <early-boot.h>
#include <mem.h>
#include <mem/PM/physalloc.h>
#include <mem/VM/virtmem.h>
#include <debug/serial.h>
#include <kmain.h>
#include <interrupts.h>
#include <debug/serial.h>



typedef void (*ctor_constructor)();
extern "C" ctor_constructor start_ctors;
extern "C" ctor_constructor end_ctors;

// The following will be our kernel's entry point.
extern "C" void _start(struct stivale2_struct *stivale2_struct) {
    // We are in a very limited enviroment right now, including the fact that the kernel heap doesnt work yet
    // (on purpose, to prevent undef. behaviour incase we accidentally call it)

    // Very first thing is we create the gdt  
    load_gdt();
    // We initialize the kernel internal heap as well, it is needed for initializing 
    __init_heap();
    // Setup serial debug output
    __init_serial();

    // Call the global constructors
    for (ctor_constructor* ctor = &start_ctors; ctor < &end_ctors; ctor++) {
        (*ctor)();
    }

    // Get stivale memory map tag
    stivale2_struct_tag_memmap* memmap_tag = (stivale2_struct_tag_memmap*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);

    // Check that the memory map is availabe
    if(memmap_tag->entries == 0) { for(;;); } // No output is setup yet, just hang
    Kernel::PM::physmem_ll* physical_mem_ll = new Kernel::PM::physmem_ll;
    physical_mem_ll->base = 0;
    physical_mem_ll->next = 0;
    physical_mem_ll->size = 0;
    physical_mem_ll->type = 0;
    Kernel::PM::physmem_ll* curr_phys_mem_ll = physical_mem_ll;
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
            case STIVALE2_MMAP_KERNEL_AND_MODULES: entry_type = PHYSMEM_LL_TYPE_KERNEL_MODULES; break;
            default: break; // Leave it with the error entry
        }

        // Perform check if the memory area is in the low area
        if(entry_type == PHYSMEM_LL_TYPE_USEABLE && memmap_tag->memmap[i].base < 0x1000) {
            entry_type = PHYSMEM_LL_TYPE_LOWMEM;
        }

        curr_phys_mem_ll->base = memmap_tag->memmap[i].base;
        curr_phys_mem_ll->type = entry_type;
        curr_phys_mem_ll->size = memmap_tag->memmap[i].length;
        curr_phys_mem_ll->next = NULL;

        // If this isnt the last one, allocate next one and change to it
        if(i != (memmap_tag->entries - 1)) {
            curr_phys_mem_ll->next = new Kernel::PM::physmem_ll;
            curr_phys_mem_ll = curr_phys_mem_ll->next;
        }
    }

    // Now initialize the physical memory allocator
    Kernel::PM::Manager::the().Init(physical_mem_ll);

    // Initialize virtual memory
    Kernel::VM::Manager::the().Init();

    // Map kernel
    curr_phys_mem_ll = physical_mem_ll;
    while(curr_phys_mem_ll) {
        if(curr_phys_mem_ll->type == PHYSMEM_LL_TYPE_KERNEL_MODULES) {
            uint64_t kernel_phys_begin = (curr_phys_mem_ll->base & 0xFFFFFFFFFF000);
            uint64_t kernel_phys_end = ((curr_phys_mem_ll->base + curr_phys_mem_ll->size) & 0xFFFFFFFFFF000);

            uint64_t kernel_virt_offset = 0xffffffff80000000;

            // Some logic incase the kernel overflows the last page
            if((curr_phys_mem_ll->base + curr_phys_mem_ll->size) & 0xFFF) { kernel_phys_end += 4 * 1024; }

            for(uint64_t current_map = kernel_phys_begin; current_map < kernel_phys_end; current_map += 4 * 1024) {
                Kernel::VM::Manager::the().MapPageEarly(current_map, kernel_phys_begin + kernel_virt_offset);
                kernel_virt_offset += 4 * 1024;
            }
        }
        curr_phys_mem_ll = curr_phys_mem_ll->next;
    }


    // Create the module pointer
    struct stivale2_struct_tag_modules* module_struct = (stivale2_struct_tag_modules*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MODULES_ID);
    size_t module_count = module_struct->module_count;
    uint8_t** module_pointers = new uint8_t*[module_count];
    uint64_t* module_sizes = new uint64_t[module_count];
    char** module_names = new char*[module_count];
    for(size_t i = 0; i < module_count; i++) {
        // These are phyiscal addresses, but since they are also a mapping in the stivale2 stuff, we can
        // simply add the virtual base to them and they will work
        module_pointers[i] = (uint8_t*)(module_struct->modules[i].begin + 0xffffffff80000000);
        module_sizes[i] = module_struct->modules[i].end - module_struct->modules[i].begin;
        module_names[i] = (char*)((uint64_t)module_struct->modules[i].string + 0xffffffff80000000);
    }

    // We can now switch to the new page table
    Kernel::VM::Manager::the().SwitchPageTables();



    // We are in a much safer place now; dereferencing null pointers will for example now crash, before it would have been identity mapped to physical memory.


    // Initialize TSS
    load_tss();

    // Initalize Interrupts
    Kernel::Interrupts::the().InitInterrupts();

    // Basic init has occured, we can call the main now
    Kernel::KernelMain(module_count, module_pointers, module_sizes, module_names);
}
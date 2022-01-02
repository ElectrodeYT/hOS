#include <stdint.h>
#include <stddef.h>
#include <stivale2.h>
#include <early-boot.h>
#include <mem.h>
#include <mem/PM/physalloc.h>
#include <mem/VM/virtmem.h>
#include <debug/klog.h>
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

    // Attempt to load the stivale2 screen terminal callback
    stivale2_term_init((stivale2_struct_tag_terminal*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_TERMINAL_ID));
    stivale2_term_write("hOS early boot\n");
    


    // Get stivale memory map tag
    stivale2_struct_tag_memmap* memmap_tag = (stivale2_struct_tag_memmap*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
    if(!memmap_tag) {
        stivale2_term_write("boot error: memmap tag not found\n");
        for(;;);
    }

    // Dump memory map
    uint64_t total_usable = 0;
    uint64_t total_kernel = 0;
    uint64_t total_bootloader = 0;
    for(size_t i = 0; i < memmap_tag->entries; i++) {
        struct stivale2_mmap_entry entry = memmap_tag->memmap[i];
        stivale2_term_write("memmap entry %i: base %x, len %x\n", i, entry.base, entry.length);
        switch(entry.type) {
            case STIVALE2_MMAP_ACPI_NVS: { stivale2_term_write("\ttype: ACPI_NVS\n"); break; }
            case STIVALE2_MMAP_ACPI_RECLAIMABLE: { stivale2_term_write("\ttype: ACPI_RECLAIMABLE\n"); break; }
            case STIVALE2_MMAP_BAD_MEMORY: { stivale2_term_write("\ttype: BAD_MEMORY\n"); break; }
            case STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE: { stivale2_term_write("\ttype: BOOTLOADER_RECLAIMABLE\n"); total_bootloader += entry.length; break; }
            case STIVALE2_MMAP_FRAMEBUFFER: { stivale2_term_write("\ttype: FRAMEBUFFER\n"); break; }
            case STIVALE2_MMAP_KERNEL_AND_MODULES: { stivale2_term_write("\ttype: KERNEL_AND_MODULES\n"); total_kernel += entry.length; break; }
            case STIVALE2_MMAP_RESERVED: { stivale2_term_write("\ttype: RESERVED\n"); break; }
            case STIVALE2_MMAP_USABLE: { stivale2_term_write("\ttype: USABLE\n"); total_usable += entry.length; break; }
            default: { stivale2_term_write("\ttype: INVALID\n"); break; }
        }
    }
    uint64_t total_ram = total_usable + total_kernel + total_bootloader;
    stivale2_term_write("total ram %i MiB, %i MiB used by kernel, %i MiB used by bootloader, %i MiB usable\n", total_ram / (1024 * 1024), total_kernel / (1024 * 1024), total_bootloader / (1024 * 1024), total_usable / (1024 * 1024));

    // Get HDDM tag
    stivale2_struct_tag_hhdm* hhdm_tag = (stivale2_struct_tag_hhdm*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_HHDM_ID);
    if(!hhdm_tag) {
        stivale2_term_write("boot error: unable to get hhdm offset\n");
        for(;;);
    }

    // Now initialize the physical memory allocator
    Kernel::PM::Init(memmap_tag, hhdm_tag->addr);

    stivale2_term_write("goodbye stivale2\n");    

    // Setup serial debug output
    __init_serial();
    
    // Initialize virtual memory
    Kernel::VM::Init(memmap_tag, hhdm_tag->addr);


    // Call the global constructors
    for (ctor_constructor* ctor = &start_ctors; ctor < &end_ctors; ctor++) {
        (*ctor)();
    }

    // Init KLog
    Kernel::KLog::the().registerCallback(Kernel::Debug::SerialPrintWrap, NULL);


    // We are in a much safer place now; dereferencing null pointers will for example now crash, before it would have been identity mapped to physical memory.

    // Get the not requried fb struct
    stivale2_struct_tag_framebuffer* fb_tag = (stivale2_struct_tag_framebuffer*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);

    // Load the gdt
    load_gdt();
    // Initialize TSS
    load_tss();

    // Initalize Interrupts
    Kernel::Interrupts::the().InitInterrupts();

    // Basic init has occured, we can call the main now
    Kernel::KernelMain(fb_tag);
}
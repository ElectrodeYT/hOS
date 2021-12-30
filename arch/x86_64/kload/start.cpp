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

    // Test the PM
    // Basically we allocate a bunch of pages (~100, but not too many) and record the last page
    // Due to how the memory layout is most likely arranged (at leas on qemu) the descriptors should be arranged
    // according to their base values
    uint64_t last_allocated_page = 0;
    for(size_t i = 0; i < 100; i++) {
        uint64_t current = Kernel::PM::AllocatePages();
        if(last_allocated_page >= current) {
            Kernel::Debug::SerialPrintf("PM test failed: page %x is lower in the memory space than %x, allocation %i\n\r", current, last_allocated_page, i);
            for(;;);
        }
        last_allocated_page = current;
    }

    // Test the VM
    uint64_t* test1 = (uint64_t*)Kernel::VM::AllocatePages(16);
    uint64_t* test2 = (uint64_t*)Kernel::VM::AllocatePages(16);
    
    memset(test1, 0xAB, 16 * 4096);
    memset(test2, 0xCD, 16 * 4096);
    const uint64_t test1_mask = 0xABABABABABABABAB;
    const uint64_t test2_mask = 0xCDCDCDCDCDCDCDCD;
    for(size_t i = 0; i < ((16 * 4096) / 8); i++) {
        if(test1[i] != test1_mask) {
            Kernel::Debug::SerialPrintf("VM test failed: uint64_t %i should be 0xABABABABABABABAB, was %x\n\r", i, test1[i]);
            for(;;);
        }
        if(test2[i] != test2_mask) {
            Kernel::Debug::SerialPrintf("VM test failed: uint64_t %i should be 0xCDCDCDCDCDCDCDCD, was %x\n\r", i, test2[i]);
            for(;;);
        }
    }
    Kernel::VM::FreePages(test2);
    Kernel::VM::FreePages(test1);
    

    // Initialize the kernel heap
    __init_heap();


    // Init KLog
    Kernel::KLog::the().registerCallback(Kernel::Debug::SerialPrintWrap, NULL);


    // We are in a much safer place now; dereferencing null pointers will for example now crash, before it would have been identity mapped to physical memory.

    // Call the global constructors
    for (ctor_constructor* ctor = &start_ctors; ctor < &end_ctors; ctor++) {
        (*ctor)();
    }

    // Load the gdt
    load_gdt();
    // Initialize TSS
    load_tss();

    // Initalize Interrupts
    Kernel::Interrupts::the().InitInterrupts();

    // Basic init has occured, we can call the main now
    Kernel::KernelMain();
}
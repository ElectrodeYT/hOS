ENTRY(_loader)
OUTPUT_FORMAT(elf32-i386)
 
SECTIONS {
    /* The kernel will live at 3GB + 1MB in the virtual
    address space, which will be mapped to 1MB in the
    physical address space. */
    . = 0xC0100000;
    
    kernel_begin = .;

    .text : AT(ADDR(.text) - 0xC0000000) {
        *(.text)
        *(.gnu.linkonce.t*)
    }

    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xC0000000) {
        start_ctors = .;
        *(SORT(.ctors*))
        end_ctors = .;
        start_dtors = .;
        *(SORT(.dtors*))
        end_dtors = .;

        *(.rodata*)
        *(.gnu.linkonce.r*)
    }

    .data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000) {
        *(.data)
    }

    .bss : AT(ADDR(.bss) - 0xC0000000) {
        _sbss = .;
        *(COMMON)
        *(.bss)
        _ebss = .;
    }

    /* Heap metadata */
    .heap_metadata (NOLOAD) : AT(ADDR(.heap_metadata) - 0xC0000000) {
        *(.heap_metadata)
    }

    /* Heap memory, used for malloc and co. */
    /* Defined as noload as not to increase the size of the binary. */
    .heap ALIGN(4K) (NOLOAD) : AT(ADDR(.heap) - 0xC0000000) {
        kernel_heap_begin = .;
        *(.heap)
        kerenel_heap_binary_end = .;
    }

    kernel_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}
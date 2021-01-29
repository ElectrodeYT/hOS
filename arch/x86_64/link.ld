KERNEL_LMA = 0x00100000; /* 2 MEG */
KERNEL_VMA = 0xffffffff80000000 + KERNEL_LMA;

ENTRY(entry)

SECTIONS {

    /* Lower half stuff */
    . = KERNEL_LMA;

    .boostrap_bss : AT(ADDR(.boostrap_bss)) {
        /* Bootstrap stack */
        *(.boostrap_bss)
    }

    /* Higher half stuff */
	. = KERNEL_VMA;


    .text : AT(ADDR(.text) - KERNEL_VMA)
    {
        _code = .;
        *(.text)
        *(.rodata*)
        . = ALIGN(4096);
    }

   .data : AT(ADDR(.data) - KERNEL_VMA)
   {
        _data = .;
        *(.data)
        . = ALIGN(4096);
   }

   .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA)
   {
       _ehframe = .;
       *(.eh_frame)
        . = ALIGN(4096);
   }

   .bss : AT(ADDR(.bss) - KERNEL_VMA)
   {
       _bss = .;
       *(.bss)

       /*
        * You usually need to include generated COMMON symbols
        * under kernel BSS section or use gcc's -fno-common
        */

        *(COMMON)
       . = ALIGN(4096);
   }

   _end = .;

   /DISCARD/ :
   {
        *(.comment)
   }
}
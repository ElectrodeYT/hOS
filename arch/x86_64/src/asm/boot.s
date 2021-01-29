// Boot code
// Gets jumped into in 32-bit protected code, sets up higher half paging, and goes up to long mode.
// This code is linked at the 1-Meg mark, with a higher-half linked trampoline to invalidate the 1-meg identity mapping, re-setup higher-half stack, and jump to kmain.


// Multiboot header

//define KERNEL_VMA   0xffffffff80000000

.code32



.section .multiboot.data

// Create multiboot2 header
multiboot_header_start:
.int 0xe85250d6
.int 0
.int multiboot_header_end - multiboot_header_start // length of header

.int (0x100000000 - (0xe85250d6 + 0 + (multiboot_header_end - multiboot_header_start)))

.short 0
.short 0
.int 0


multiboot_header_end:

.section .text
.global entry
entry:
    // Setup early stack
    mov early_stack_top, %esp
    // Setup serial
    call bootstrap_setup_serial
    // Print
    mov boot_32_string, %ecx
    call bootstrap_print
    hlt

// Prints to serial console (presumed setup) until null is reached.
// Pointer to text in ecx
bootstrap_print:
    // Save registers
    push %eax
    push %edx
    // Setup port
    mov $0x3F8, %dx
    .print_loop:
    // Read byte into al
    mov (%ecx), %al
    // Check if its the null terminator
    test %al, %al
    // Return if it is
    je .end
    // Shove it out
    out %al, %dx
    inc %ecx
    .end:
    pop %edx
    pop %eax
    ret

// Setup serial port.
bootstrap_setup_serial:
    // Shitton of I/O
    push %eax
    push %edx
    // Time to send some bytes
    mov $0x3F9, %dx // PORT + 1
    mov $0x00, %al  // data
    outb %al, %dx
    mov $0x3FB, %dx // PORT + 3
    mov $0x80, %al  // data
    outb %al, %dx
    mov $0x3F8, %dx // PORT + 0
    mov $0x03, %al  // data
    outb %al, %dx
    mov $0x3F9, %dx // PORT + 1
    mov $0x00, %al  // data
    outb %al, %dx
    mov $0x3FB, %dx // PORT + 3
    mov $0x03, %al  // data
    outb %al, %dx
    mov $0x3FA, %dx // PORT + 2
    mov $0xC7, %al  // data
    outb %al, %dx
    mov $0x3FC, %dx // PORT + 4
    mov $0x0B, %al  // data
    outb %al, %dx
    mov $0x3FC, %dx // PORT + 4
    mov $0x1E, %al  // data
    outb %al, %dx
    mov $0x3FC, %dx // PORT + 4
    mov $0x0F, %al  // data
    outb %al, %dx
    
    pop %edx
    pop %eax

boot_32_string:
    .asciz "Hello from 32-bit bootstrap\n\r"//

.section .boostrap_bss
early_stack_top:
    .skip 32
early_stack_bottom:

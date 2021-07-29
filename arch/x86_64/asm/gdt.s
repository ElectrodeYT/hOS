// has a very basic segment selecter flusher in it
.section .text
.global flush_segments
flush_segments:
    pushq %rax
    pushq $0x08
    pushq $.reload_cs
    retfq
    .reload_cs:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    popq %rax
    ret

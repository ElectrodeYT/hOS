// has a very basic segment selecter flusher in it

.global flush_segments
flush_segments:
    push %rax
    push $0x08
    push .reload_cs
    retfq
    .reload_cs:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    pop %rax
    ret

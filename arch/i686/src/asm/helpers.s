; This file contains various assembly helpers.

; pretty simple
global flush_gdt
flush_gdt:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush
    .flush:
    ret
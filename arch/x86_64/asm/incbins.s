// this file basically only has incbins in int
.section .rodata
.global testa_start
testa_start:
.incbin "sysroot/testa.elf"
.global testa_end
testa_end:
.global testb_start
testb_start:
.incbin "sysroot/testb.elf"
.global testb_end
testb_end:
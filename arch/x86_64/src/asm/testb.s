.section .text
.global testb_start
.org 0x8000
testb_start:
movq $1, %rax
movq $string, %rbx
movq $1, %rcx
int $0x80
jmp testb_start

string:
.asciz "B"


.global testb_end
testb_end:
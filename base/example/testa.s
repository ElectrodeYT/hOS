.global _start
.section .text
_start:
movq $2, %rax
movq $10, %rbx
int $0x80


aloop:
movq $1, %rax
movq $string, %rbx
movq $1, %rcx
int $0x80
jmp aloop

string:
.asciz "A"


testa_end:

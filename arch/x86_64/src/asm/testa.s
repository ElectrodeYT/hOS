.section .text
.global testa_start
.org 0x8000
testa_start:
// Test the memmap syscall
movq $2, %rax
movq $1, %rbx
int $0x80



aloop:
movq $1, %rax
movq $string, %rbx
movq $1, %rcx
int $0x80
jmp aloop

string:
.asciz "A"


.global testa_end
testa_end:
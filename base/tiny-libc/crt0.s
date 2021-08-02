.global _start
.section .text
_start:
// Setup stack frame
movq $0, %rbp
pushq %rbp
pushq %rbp
movq %rsp, %rbp

// Save argc, argv
pushq %rdi
pushq %rsi

call _init

// Restore argc, argv
popq %rsi
popq %rdi

call main
call _fini

// exit
movq %rax, %rbx
movq $4, %rax
int $0x80

.global _start
.section .text
_start:
// We first need to allocate a stack
// To do this we call mmap with requested size = 0x4000 -> 16 kilobytes
movq $2, %rax  // mmap
movq $0x4000, %rbx // size
int $0x80
// rax should now have the address in it

// Setup stack frame
movq %rax, %rsp
addq $0x3000, %rsp
movq $0, %rbp
pushq %rbp
pushq %rbp
movq %rsp, %rbp

// call _init
call main


// TODO: exit
loop_forever:
jmp loop_forever

.macro int_err n
.align 16
.global isr\n
isr\n:
	pushq $\n
    jmp isr_common
.endm

.macro int_noerr n
.align 16
.global isr\n
isr\n:
	pushq $0
	pushq $\n
    jmp isr_common
.endm

.macro int_irq n
.align 16
.global irq\n
irq\n:
	push $0 // Push fake error so that ISRRegisters can be reused
	pushq $\n
    jmp irq_common
.endm



isr_common:
    // Push all registers
	pushq %rbp
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	mov %rsp, %rdi
.extern isr_main
	call isr_main
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rax
	popq %rbx
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
	popq %rbp
	add $16, %rsp // Info field
	iretq


irq_common:
    // Push all registers
	pushq %rbp
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	mov %rsp, %rdi
.extern irq_main
	call irq_main
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rax
	popq %rbx
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
	popq %rbp
	add $16, %rsp // Info field
	iretq
    
int_noerr 0
int_noerr 1
int_noerr 2
int_noerr 3
int_noerr 4
int_noerr 5
int_noerr 6
int_noerr 7
int_err 8
int_noerr 9
int_err 10
int_err 11
int_err 12
int_err 13
int_err 14
int_noerr 15
int_noerr 16
int_err 17
int_noerr 18
int_noerr 19
int_noerr 20
int_noerr 21
int_noerr 22
int_noerr 23
int_noerr 24
int_noerr 25
int_noerr 26
int_noerr 27
int_noerr 28
int_noerr 29
int_err 30
int_noerr 31

int_irq 0
int_irq 1
int_irq 2
int_irq 3
int_irq 4
int_irq 5
int_irq 6
int_irq 7
int_irq 8
int_irq 9
int_irq 10
int_irq 11
int_irq 12
int_irq 13
int_irq 14
int_irq 15

int_noerr 128

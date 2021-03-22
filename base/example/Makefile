AS := nasm

all:
	@echo [BASE-ASM]	test.s
	@$(AS) -fbin test.s -o example.mod
clean:
	@rm example.mod 2> /dev/null; true
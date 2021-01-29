# Arch makefile for x86_64

base_folder := ../../

kernel := hKern.elf
image := hOS.img

cd_root := boot/cd-root

cd_root_kernel := $(cd_root)/boot/kernel.elf

.PHONY: create-image qemu

create-image $(image): $(kernel)
	@# Copy relevant files into cdrom-root
	@echo [COPY] $(kernel)
	@cp $(kernel) $(cd_root_kernel)

	@# Create ISO with grubmkrescue
	@echo [ISO] $(image)
	@grub-mkrescue $(cd_root) -o $(image)

qemu: $(image)
	@qemu-system-x86_64 -cdrom $(image) -m 256

qemu-debug: $(image)
	@gdb -x boot/gdb.gdb -tui

clean:
	@rm $(image) 2> /dev/null; true
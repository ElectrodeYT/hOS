# Arch makefile for x86_64

base_folder := ../../

kernel := hKern.elf
image := hOS.img

limine_config := boot/limine.cfg

.PHONY: create-image qemu

create-image $(image): $(kernel)
	@# Create the image
	@rm -f $(image)
	@dd if=/dev/zero bs=1M count=0 seek=64 of=$(image)
	@parted -s $(image) mklabel gpt
	@parted -s $(image) mkpart primary 2048s 100%
	@echfs-utils -g -p0 $(image) quick-format 512
	@echfs-utils -g -p0 $(image) import $(kernel) kernel.elf
	@echfs-utils -g -p0 $(image) import $(limine_config) limine.cfg
	@limine-install $(image)

qemu: $(image)
	@qemu-system-x86_64 -hda $(image) -m 256

qemu-debug: $(image)
	@gdb -x boot/gdb.gdb -tui

clean:
	@rm $(image) 2> /dev/null; true
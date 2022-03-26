# Arch makefile for x86_64

base_folder := ../../

kernel := hKern.elf
image := hOS.img

# Stuff to import into image
limine_config := ../../sysroot/boot/limine.cfg
limine_sys := /usr/share/limine/limine.sys
libc := ../../sysroot/usr/lib/libc.so
ld := ../../sysroot/usr/lib/ld.so

.PHONY: create-image qemu

create-image $(image): $(kernel)
	@# Create the image
	@rm -f $(image)
	@dd if=/dev/zero bs=1M count=0 seek=64 of=$(image)
	@parted -s $(image) mklabel gpt
	@parted -s $(image) mkpart primary 2048s 100%
	@echfs-utils -g -p0 $(image) quick-format 512
	@echfs-utils -g -p0 $(image) mkdir usr
	@echfs-utils -g -p0 $(image) mkdir usr/include
	@echfs-utils -g -p0 $(image) mkdir usr/lib
	@echfs-utils -g -p0 $(image) mkdir lib
	@echfs-utils -g -p0 $(image) import $(kernel) kernel.elf
	@echfs-utils -g -p0 $(image) import $(limine_config) limine.cfg
	@echfs-utils -g -p0 $(image) import $(limine_sys) limine.sys
	@echfs-utils -g -p0 $(image) import $(libc) usr/lib/libc.so
	@echfs-utils -g -p0 $(image) import $(ld) usr/lib/ld.so
	@echfs-utils -g -p0 $(image) import $(ld) lib/ld64.so.1
	@echfs-utils -g -p0 $(image) import ../../sysroot/init init
	@echfs-utils -g -p0 $(image) import ../../sysroot/hello.txt hello.txt
	@echfs-utils -g -p0 $(image) import ../../sysroot/dev/internal-kernel-mountpoint.txt dev/internal-kernel-mountpoint.txt
	@limine-install $(image)

qemu: $(image)
	@qemu-system-x86_64 -drive format=raw,file=$(image) -m 256 -d int -no-reboot -no-shutdown --enable-kvm

qemu-debug: $(image)
	@gdb -x ../../sysroot/boot/gdb.gdb -tui

qemu-init-debug: $(image)
	@gdb -x ../../sysroot/boot/gdb-init.gdb -tui

qemu-ld-init-debug: $(image)
	@gdb -x ../../sysroot/boot/gdb-ld-init.gdb -tui

clean:
	@rm $(image) 2> /dev/null; true
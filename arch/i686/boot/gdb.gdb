file boot/cd-root/boot/kernel.elf
target remote | qemu-system-i386 -cdrom hOS.img -S -gdb stdio -m 256
break kmain


continue
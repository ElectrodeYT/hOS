file boot/cd-root/boot/kernel.elf
target remote | qemu-system-x86_64 -cdrom hOS.img -S -gdb stdio -m 256
break entry
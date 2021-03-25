file hKern.elf
target remote | qemu-system-x86_64 -hda hOS.img -S -gdb stdio -m 256
break _start
set print pretty
continue
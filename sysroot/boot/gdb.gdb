file hKern.elf
target remote | qemu-system-x86_64 -drive format=raw,file=hOS.img -S -gdb stdio -m 256 -no-reboot -no-shutdown
break _start
set print pretty
continue
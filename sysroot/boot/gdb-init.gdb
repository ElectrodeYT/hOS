file ../../sysroot/init
target remote | qemu-system-x86_64 -drive format=raw,file=hOS.img -S -gdb stdio -m 256 -no-reboot -no-shutdown
break main
set print pretty
continue

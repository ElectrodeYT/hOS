file ../../sysroot/init
add-symbol-file ../../sysroot/usr/lib/ld.so 0x40000000
target remote | qemu-system-x86_64 -drive format=raw,file=hOS.img -S -gdb stdio -m 256 -no-reboot -no-shutdown
break _start
set print pretty
continue

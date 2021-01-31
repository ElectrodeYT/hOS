file boot/cd-root/boot/kernel.elf
target remote | qemu-system-i386 -cdrom hOS.img -S -gdb stdio -m 256
break kmain
break isr_32
break isr_33
break isr_34
break isr_35
break isr_36
break isr_37
break isr_38
break isr_39
break isr_40
break isr_41
break isr_42
break isr_43
break isr_44
break isr_45
break isr_46
break isr_47

continue
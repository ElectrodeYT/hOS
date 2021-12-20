# hOS
OS Project. Dont expect much lol.

64 bit x86

written in c++.

# Cloning
This project uses git submodules, so clone it like this:
```
git clone https://github.com/electrodeyt/hOS --recursive-submodules
```

# Dependencies
echfs-utils and limine are required for the qemu image creation.

# Before doing anything
Source the change-path.sh script:
´´´
source change-path.sh
´´´
Then compile the toolchain in compiler:
´´´
cd compiler
./compile-crosscompiler.sh
´´´
This will download binutils+gcc, patch them, and then compile them (with -j$(nproc))

# Errors
### limine.sys
The Makefile in arch/x86_64 assumes the limine.sys file (required for the limine bootloader) is located at /usr/share/limine/limine.sys. If this is different, change the limine_sys variable in arch/x86_64/Makefile.
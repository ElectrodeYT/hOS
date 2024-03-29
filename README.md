# hOS
A OS Kernel. Uses mlibc. Is 64-bit.

# Philosophy
Has changed a lot during development, but at this point i want it to become a monolithic/hybrid kernel, using something like UBI/EDI/CDI for "less important" drivers (probably wont ever happen)
Used to be a microkernel, is being changed

# Cloning
This project uses git submodules, so clone it like this:
```
git clone https://github.com/electrodeyt/hOS --recursive-submodules
```

# Dependencies
`echfs-utils` and `limine` are required for the qemu image creation.

# Before doing anything
Source the `change-path.sh` script:
```
source change-path.sh
```
Then compile the toolchain in the `compiler` folder:
```
cd compiler
./compile-crosscompiler.sh
```
This will download binutils+gcc, patch them, and then compile them (with -j$(nproc))

# Credits
For now, the kernel heap allocator used is Durand's liballoc. The license file provided is in `arch/x86_64/mem/heap/liballoc`, and is under a seperate license then the rest of the kernel.
# Errors
### limine.sys
The Makefile in `arch/x86_64` assumes the `limine.sys` file (required for the limine bootloader) is located at `/usr/share/limine/limine.sys`. If this is different, change the `limine_sys` variable in `arch/x86_64/Makefile`.

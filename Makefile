ARCH := i686

C := $(ARCH)-elf-gcc
CPP := $(ARCH)-elf-g++
AS := nasm

C_ARGS := -ffreestanding -O2 -Wall -Wextra -g -Iarch/$(ARCH)/inc
CPP_ARGS := -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -fno-stack-protector -g -Iarch/$(ARCH)/inc
AS_ARGS := -f elf32 -g

assembly := $(shell find arch/$(ARCH)/src/asm/ -name *.s)
c := $(shell find arch/$(ARCH)/src/ -name *.c)
cpp := $(shell find arch/$(ARCH)/src/ -name *.cpp)

assembly_o := $(filter-out crtn.o,$(filter-out crti.o,$(assembly:%.s=%.o)))

c_o := $(c:%.c=%.o)
cpp_o := $(cpp:%.cpp=%.o)

linker_file := arch/$(ARCH)/link.ld
kernel := arch/$(ARCH)/hKern.elf
boot_image := arch/$(ARCH)/hKern.img
# Objects we need to link
objects := $(assembly_o) $(c_o) $(cpp_o)

.PHONY: all create-image link qemu qemu-debug cloc

%.o: %.s
	@echo "[ASM]		" $< $@
	@$(AS) $< -o $@ $(AS_ARGS)

%.o: %.c
	@echo "[C]		" $< $@
	@$(C) -c $< -o $@ $(C_ARGS)

%.o: %.cpp
	@echo "[CPP]		" $< $@
	@$(CPP) -c $< -o $@ $(CPP_ARGS)

link $(kernel): $(objects)
	@echo "[LINK]		$(kernel)"
	@$(C) -T $(linker_file) -o $(kernel) -ffreestanding -nostdlib -O2 $(objects) -lgcc


create-image $(boot_image): $(kernel)
	@$(MAKE) -C arch/$(ARCH) create-image

all: $(boot_image)

qemu: $(boot_image)
	@$(MAKE) -C arch/$(ARCH) qemu

qemu-debug: $(boot_image)
	@$(MAKE) -C arch/$(ARCH) qemu-debug

clean:
	@rm $(objects) $(kernel) $(boot_image) 2> /dev/null; true
	@$(MAKE) -C arch/$(ARCH) clean

cloc:
	@cloc arch/$(ARCH)/src arch/$(ARCH)/inc
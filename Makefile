ARCH := x86_64

C := $(ARCH)-elf-gcc
CPP := $(ARCH)-elf-g++
AS := $(ARCH)-elf-as

C_ARGS := -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O2 -Wall -Wextra -pedantic -g -Iarch/$(ARCH)/inc -Iagnostic/inc
CPP_ARGS := -ffreestanding -O2 -Wall -Wextra -pedantic -fno-exceptions -fno-rtti -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -mgeneral-regs-only -mno-red-zone -fno-stack-protector -fPIC -g -Iarch/$(ARCH)/inc -Iagnostic/inc
AS_ARGS := -g

assembly := $(shell find arch/$(ARCH)/src/asm/ -name *.s)
c := $(shell find arch/$(ARCH)/src/ agnostic/src/ -name *.c)
cpp := $(shell find arch/$(ARCH)/src/ -name *.cpp)

assembly_o := $(filter-out crtn.o,$(filter-out crti.o,$(assembly:%.s=%.o)))

c_o := $(c:%.c=%.o)
cpp_o := $(cpp:%.cpp=%.o)

linker_file := arch/$(ARCH)/link.ld
kernel := arch/$(ARCH)/hKern.elf
boot_image := arch/$(ARCH)/hKern.img
# Objects we need to link
objects := $(assembly_o) $(c_o) $(cpp_o)

# Services we need to boot
services := base/example/example.mod

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


create-image $(boot_image): $(services) $(kernel)
	@$(MAKE) -C arch/$(ARCH) create-image

compile-services $(services):
	@$(MAKE) -C base all ARCH=$(ARCH)

all: $(boot_image) $(services)

qemu: $(boot_image)
	@$(MAKE) -C arch/$(ARCH) qemu

qemu-debug: $(boot_image)
	@$(MAKE) -C arch/$(ARCH) qemu-debug

clean:
	@rm $(objects) $(kernel) $(boot_image) 2> /dev/null; true
	@$(MAKE) -C arch/$(ARCH) clean
	@$(MAKE) -C base clean

cloc:
	@cloc arch/$(ARCH)/src arch/$(ARCH)/inc
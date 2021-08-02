clib := tiny-clib

SYSROOT := ../../sysroot
AS := $(ARCH)-hos-as
LD := $(ARCH)-hos-ld
C := $(ARCH)-hos-gcc
CARGS := -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O2 -Wall -Wextra -g  -I$(SYSROOT)/usr/include
OBJCPY := $(ARCH)-elf-objcopy
sources := testa.c testb.c
objects := testa.elf testb.elf
objects_sysroot := $(addprefix $(SYSROOT)/, $(objects))

%.elf: %.s
	@echo "[BASE-ASM]		" $< $@
	@$(AS) $< -o $@.o
	@$(LD) $@.o -o $@

%.elf: %.c
	@echo "[BASE-C]		" $< $@
	@$(C) -c $< -o $@.o $(CARGS) -I../tiny-libc/inc
	@$(C) -O2 $@.o -o $@ -static -lgcc

$(SYSROOT)/%.elf: %.elf
	@cp $< $@

all: $(objects) $(objects_sysroot)

clean:
	@rm $(shell find . -name "*.elf*") $(objects_sysroot) 2> /dev/null; true
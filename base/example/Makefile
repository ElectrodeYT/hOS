clib := tiny-clib

SYSROOT := ../../sysroot
C := $(realpath ../../compiler/opt/bin/$(ARCH)-hos-gcc)
CPP := $(realpath ../../compiler/opt/bin/$(ARCH)-hos-g++)
AS := $(realpath ../../compiler/opt/bin/$(ARCH)-hos-as)
LD := $(realpath ../../compiler/opt/bin/$(ARCH)-hos-ld)
STRIP := $(ARCH)-hos-strip
CARGS := -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O2 -Wall -Wextra -g  -I$(SYSROOT)/usr/include
OBJCPY := $(ARCH)-elf-objcopy
sources := init.c
objects := init.elf
objects_sysroot := $(addprefix $(SYSROOT)/, $(basename $(objects)))

%.elf: %.s
	@echo "[BASE-ASM]		" $< $@
	@$(AS) $< -o $@.o
	@$(LD) $@.o -o $@

%.elf: %.c
	@echo "[BASE-C]		" $< $@
	@$(C) -c $< -o $@.o $(CARGS)
	@$(C) -O2 $@.o -o $@ -lgcc 
	@#$(STRIP) $@

$(objects_sysroot): ${objects}
	@echo "[CP]			"$< $@
	@cp $< $@

all: $(objects) $(objects_sysroot)

clean:
	@rm $(shell find . -name "*.elf*") $(objects_sysroot) 2> /dev/null; true
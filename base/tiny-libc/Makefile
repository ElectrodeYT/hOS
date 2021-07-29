SYSROOT := ../../sysroot
AS := $(ARCH)-elf-as
LD := $(ARCH)-elf-ld
C := $(ARCH)-elf-gcc
CARGS := -ffreestanding -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O2 -Wall -Wextra -I. -g
AR := $(ARCH)-elf-ar
OBJCPY := $(ARCH)-elf-objcopy

assembly := $(shell find asm -name "*.s")
c := $(shell find . -name "*.c")
assembly_o := $(filter-out crtn.o,$(filter-out crti.o,$(assembly:%.s=%.o)))
c_o := $(c:%.c=%.o)

objects := $(assembly_o) $(c_o)

crt-shit := asm/crt0.o asm/crti.s asm/crtn.s

static-lib := ../../sysroot/usr/lib/libtiny-libc.a

headers := $(shell find * -name "*.h")
headers_in_sysroot := $(addprefix $(SYSROOT)/usr/include/, $(headers))

all: $(objects) $(crt-shit) $(static-lib) $(headers_in_sysroot)


%.o: %.s
	@echo "[BASE-ASM]		" $< $@
	@$(AS) $< -o $@ -g

%.o: %.c
	@echo "[BASE-C]		" $< $@
	@$(C) -c $< -o $@ $(CARGS)

$(SYSROOT)/usr/include/%.h: %.h
	@cp $< $@

link $(static-lib): $(objects) $(crt-shit)
	@echo "[BASE-AR]		"  $(static-lib)
	@$(AR) rcs $(static-lib) asm/crt0.o asm/crti.o $(objects) asm/crtn.o


clean:
	@rm $(objects) $(headers_in_sysroot) $(static-lib) 2> /dev/null; true
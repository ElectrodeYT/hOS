SYSROOT := ../../sysroot
AS := $(ARCH)-hos-as
LD := $(ARCH)-hos-ld
C := $(ARCH)-hos-gcc
CARGS := -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O2 -Wall -Wextra -I. -g
AR := $(ARCH)-hos-ar
OBJCPY := $(ARCH)-hos-objcopy

assembly := $(shell find . -name "*.s")
c := $(shell find . -name "*.c")
assembly_o := $(filter-out crt0.o, $(filter-out crtn.o,$(filter-out crti.o,$(assembly:%.s=%.o))))
c_o := $(c:%.c=%.o)

objects := $(assembly_o) $(c_o)

crt-shit := crt0.o crti.s crtn.s

static-lib := ../../sysroot/usr/lib/libc.a

headers := $(shell find * -name "*.h")
headers_in_sysroot := $(addprefix $(SYSROOT)/usr/include/, $(headers))

crt_in_sysroot := $(addprefix $(SYSROOT)/usr/lib/, $(crt-shit:%.s=%.o))

all: $(objects) $(crt-shit) $(static-lib) $(headers_in_sysroot)

install-headers: $(headers_in_sysroot)

%.o: %.s 
	@echo "[BASE-ASM]		" $< $@
	@$(AS) $< -o $@ -g

%.o: %.c
	@echo "[BASE-C]		" $< $@
	@$(C) -c $< -o $@ $(CARGS)

$(SYSROOT)/usr/include/%.h: %.h
	@echo "[CP]			" $< $@
	@cp -f $< $@

$(SYSROOT)/usr/lib/%.o: %.o
	@echo "[CP]			" $<
	@cp -f $< $(SYSROOT)/usr/lib

link $(static-lib): $(crt_in_sysroot) $(objects) $(crt-shit)
	@echo "[BASE-AR]		"  $(static-lib)
	@$(AR) rcs $(static-lib) $(objects)


clean:
	@rm $(objects) $(crt_in_sysroot) $(headers_in_sysroot) $(static-lib) 2> /dev/null; true
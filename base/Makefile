BOOT_LIBS := ../sysroot/usr/include/libtiny-libc.a
BOOT_APPS := ../sysroot/testa.elf ../sysroot/testb.elf

all: $(BOOT_APPS) $(BOOT_LIBS)

compile-example $(BOOT_APPS): $(BOOT_LIBS)
	@$(MAKE) -C example all

compile-libs $(BOOT_LIBS):
	@$(MAKE) -C tiny-libc all


clean:
	@$(MAKE) -C example clean
	@$(MAKE) -C tiny-libc clean
BOOT_LIBS := ../sysroot/usr/lib/libc.a
BOOT_APPS := ../sysroot/init

all: $(BOOT_APPS) $(BOOT_LIBS)

compile-example $(BOOT_APPS): $(BOOT_LIBS)
	@$(MAKE) -C example all

compile-libs $(BOOT_LIBS):
	@./meson-mlibc.sh
	@ninja -C mlibc-build
	@ninja -C mlibc-build install


clean:
	@$(MAKE) -C example clean
	@# $(MAKE) -C tiny-libc clean
	rm -rf ../sysroot/usr/include/*; true 2> /dev/null
	rm -rf ../sysroot/usr/lib/*; true 2> /dev/null
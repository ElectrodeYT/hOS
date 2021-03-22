#include <stdint.h>
#include <stddef.h>
#include <stivale2.h>
#include <early-boot.h>


// The following will be our kernel's entry point.
extern "C" void _start(struct stivale2_struct *stivale2_struct) {
    // Let's get the framebuffer tag.
    struct stivale2_struct_tag_framebuffer* fb_hdr_tag;
    fb_hdr_tag = (stivale2_struct_tag_framebuffer*)stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);

    // Check if the tag was actually found.
    if (fb_hdr_tag == NULL) {
        // It wasn't found, just hang...
        for (;;) {
            asm ("hlt");
        }
    }
    
    load_gdt();

    // Let's get the address of the framebuffer.
    uint8_t *fb_addr = (uint8_t *)fb_hdr_tag->framebuffer_addr;

    // Let's try to paint a few pixels white in the top left, so we know
    // that we booted correctly.
    //for (size_t i = 0; i < 128; i++) {
    //    fb_addr[i] = 0xff;
    //}

    // We're done, just hang...
    for (;;) {
        asm ("hlt");
    }
}
#include <kernel-drivers/Stivale2GraphicsTerminal.h>
#include <debug/klog.h>
#include <mem/VM/virtmem.h>
#include <errno.h>
#include <kernel-drivers/kernel-fonts/vincent.h>
#include <mem.h>

namespace Kernel {

int Stivale2GraphicsTerminal::read_drv(char* buf, size_t len) {
    if(!supported_fb) { return -ENOSYS; }
    return -ENOSYS;
    (void)buf; (void)len;
}

int Stivale2GraphicsTerminal::write_drv(const char* buf, size_t len) {
    if(!supported_fb) { return -ENOSYS; }
    if(bpp == 32) {
        for(size_t i = 0; i < len; i++) {
            switch(buf[i]) {
                case '\r': break;
                case '\n': {
                    cursor_y++;
                    if(cursor_y == max_y) { CopyUpOne(); cursor_y--; }
                    cursor_x = 0;
                    break;
                }
                default: {
                    char c = buf[i];
                    // Calculate offset to top left of char
                    uint32_t* pixels = (uint32_t*)(fb + (pitch * (cursor_y * char_height)) + (cursor_x * char_width * 4));
                    for(size_t y = 0; y < char_height; y++) {
                        for(size_t x = 0; x < char_width; x++) {
                            if(vincent_data[c & ~(1 << 8)][y] & (0b10000000 >> x)) {
                                *pixels = fore;
                            }
                            pixels++;
                        }
                        pixels = (uint32_t*)((uint8_t*)(pixels) + pitch);
                        pixels -= char_width;
                    }
                    cursor_x++;
                    if(cursor_x == max_x) {
                        cursor_x = 0;
                        cursor_y++;
                        if(cursor_y == max_y) { CopyUpOne(); cursor_y--; }
                    }
                }
            }
        }
    }
    return 0;
}

void Stivale2GraphicsTerminal::CopyUpOne() {
    memcopy(fb + pitch, fb, pitch * (height - char_height));
}

bool Stivale2GraphicsTerminal::init(stivale2_struct_tag_framebuffer* fb_tag) {
    fb = (uint8_t*)(fb_tag->framebuffer_addr);
    width = fb_tag->framebuffer_width;
    height = fb_tag->framebuffer_height;
    pitch = fb_tag->framebuffer_pitch;
    bpp = fb_tag->framebuffer_bpp;

    max_x = width / char_width;
    max_y = height / char_height;

    KLog::the().printf("Stivale2GraphicsTerminal: bpp %i, width %i, height %i, pitch %i\n\r", bpp, width, height, pitch);
    if(bpp != 32) {
        KLog::the().printf("Stivale2GraphicsTerminal: bpp %i is unsupported\n\r", bpp);
        supported_fb = false;
        return false;
    }
    supported_fb = true;
    return true;
}

void Stivale2GraphicsTerminal::Clear() {
    memset(fb, 0, height * pitch);
    cursor_x = 0;
    cursor_y = 0;
}

}
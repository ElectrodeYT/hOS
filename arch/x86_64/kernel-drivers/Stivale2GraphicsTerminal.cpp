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
    acquire(&mutex);
    uint64_t top_x = 0;
    uint64_t top_y = 0;
    uint64_t bottom_x = 0;
    uint64_t bottom_y = 0;

    bool top_bottom = false;

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
                case '\t': {
                    cursor_x += 4;
                    if(cursor_x >= max_x) {
                        cursor_x = 0;
                        cursor_y++;
                        if(cursor_y == max_y) { CopyUpOne(); cursor_y--; }
                    }
                    break;
                }
                case '\033': {
                    // TODO: actually check the buffer length
                    // TODO: make a good version of this
                    i++;
                    if(buf[i] != '[') { continue; }
                    i++;
                    if(buf[i] != '3') { continue; }
                    i++;
                    if(buf[i + 1] != 'm') { continue; }
                    switch(buf[i]) {
                        case '0': { fore = 0xFF000000; break; }
                        case '1': { fore = 0xFFAA0000; break; }
                        case '2': { fore = 0xFF00AA00; break; }
                        case '3': { fore = 0xFFAA5500; break; }
                        case '4': { fore = 0xFF0000AA; break; }
                        case '5': { fore = 0xFFAA00AA; break; }
                        case '6': { fore = 0xFF00AAAA; break; }
                        case '7':
                        default: { fore = 0xFFAAAAAA; break; }
                    }
                    i++;
                    break;
                }
                default: {
                    char c = buf[i];
                    // Calculate offset to top left of char
                    uint32_t* pixels = (uint32_t*)(fb_buffer + (pitch * (cursor_y * char_height)) + (cursor_x * char_width * 4));
                    // Check the top, bottom variables and see if we need to update them
                    if(top_bottom) {
                        if((cursor_x * char_width) < top_x || (cursor_y * char_height) < top_y) {
                            top_x = cursor_x * char_width;
                            top_y = cursor_y * char_height;
                        }
                        if(((cursor_x + 1) * char_width) > bottom_x || ((cursor_y + 1) * char_height) > bottom_y) {
                            bottom_x = (cursor_x + 1) * char_width;
                            bottom_y = (cursor_y + 1) * char_height;
                        }
                    } else {
                        top_x = cursor_x * char_width;
                        top_y = cursor_y * char_height;
                        bottom_x = (cursor_x + 1) * char_width;
                        bottom_y = (cursor_y + 1) * char_height;
                        top_bottom = true;
                    }
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
                    if(cursor_x >= max_x) {
                        cursor_x = 0;
                        cursor_y++;
                        if(cursor_y == max_y) { CopyUpOne(); cursor_y--; }
                    }
                }
            }
        }
    }
    // Copy it over to fb
    uint64_t length = bottom_x - top_x;
    for(size_t y = top_y; y < bottom_y; y++) {
        memcopy(fb_buffer + (y * pitch) + (top_x * (bpp / 8)), fb + (y * pitch) + (top_x * (bpp / 8)), length * (bpp / 8));
    }
    release(&mutex);
    return 0;
}

void Stivale2GraphicsTerminal::CopyUpOne() {
    memcopy(fb_buffer + (pitch * char_height), fb_buffer, pitch * (height - char_height));
    memset(fb_buffer + (pitch * (height - char_height)), 0, pitch * char_height);
    memcopy(fb_buffer, fb, height * pitch);
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

    // Allocate buffer
    fb_buffer = (uint8_t*)VM::AllocatePages((height * pitch) / 4096);

    supported_fb = true;
    return true;
}

void Stivale2GraphicsTerminal::Clear() {
    acquire(&mutex);
    memset(fb, 0, height * pitch);
    cursor_x = 0;
    cursor_y = 0;
    release(&mutex);
}

}
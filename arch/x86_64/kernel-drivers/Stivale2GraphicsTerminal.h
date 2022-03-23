#ifndef STIVALE2GRAPHICSTERMINAL_H
#define STIVALE2GRAPHICSTERMINAL_H
#include <CPP/mutex.h>
#include <kernel-drivers/CharDevices.h>
#include <stivale2.h>
#include <stdint.h>

namespace Kernel {

struct Stivale2GraphicsTerminal : public CharDevice {
    int read_drv(char* buf, size_t len) override;
    int write_drv(const char* buf, size_t len) override;

    bool init(stivale2_struct_tag_framebuffer* fb_tag);

    void Clear();
private:
    void CopyUpOne();
    void Present();

    uint64_t cursor_x = 0;
    uint64_t cursor_y = 0;

    uint64_t max_x = 0;
    uint64_t max_y = 0;

    uint8_t* fb;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint64_t bpp;

    uint8_t* fb_buffer;

    const size_t char_width = 8;
    const size_t char_height = 8;

    uint32_t fore = 0xFFAAAAAA;

    bool supported_fb = false;

    mutex_t mutex;
};

}

#endif
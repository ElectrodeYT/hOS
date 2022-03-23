#include <stdint.h>
#include <debug/serial.h>
#include <debug/klog.h>
#include <stdarg.h>
#include <CPP/string.h>

namespace Kernel {

void KLog::printf(const char* fmt, ...) {
    // This printf is very simple, so we dont really care much about it
    acquire(&mutex);
    puts("\033[35m[KLog] \033[37m");
    va_list args;
    va_start(args, fmt);
    char* ptr = (char*)fmt;
    // We use a buffer system to improve speed
    const int char_buf_size = 32;
    char char_buf[char_buf_size + 1];
    char* char_buf_ptr = char_buf;
    while(*ptr != '\0') {
        if(*ptr == '%') {
            // Flush buffer
            if(char_buf != char_buf_ptr) {
                (*char_buf_ptr) = 0;
                puts(char_buf);
                char_buf_ptr = char_buf;
            }
            ptr++;
            switch(*ptr) {
                case '\0': return;
                case 'i':
                case 'd': {
                    char buf[32];
                    itoa(va_arg(args, int), buf, 10);
                    puts(buf);
                    break;
                }
                case 'x':
                case 'X': {
                    uint64_t i = va_arg(args, uint64_t);
                    puts("0x");
                    char nibbles[17];
                    nibbles[16] = 0;
                    for(int nibble = 15; nibble >= 0; nibble--) {
                        uint8_t val = (i & ((uint64_t)0xF << (nibble * 4))) >> (nibble * 4);
                        nibbles[15 - nibble] = "0123456789ABCDEF"[val];
                    }
                    puts(nibbles);
                    break;
                }
                case 's': {
                    puts(va_arg(args, const char*));
                    break;
                }
                case 'c': {
                    char c = (char)(va_arg(args, int));
                    char temp_string[2] = { c, 0 };
                    puts(temp_string);
                    break;
                }
                default: continue;
            }
            ptr++;
        } else {
            (*char_buf_ptr++) = *ptr++;
            if((char_buf + char_buf_size) == char_buf_ptr) { char_buf[char_buf_size] = 0; puts(char_buf); char_buf_ptr = char_buf; }
        }
    }
    if(char_buf != char_buf_ptr) {
        (*char_buf_ptr) = 0;
        puts(char_buf);
    }
    puts("\033[37m");
    release(&mutex);
}

void KLog::puts(const char* str) {
    for(size_t i = 0; i < callbacks.size(); i++) {
        callbacks.at(i)(arguments.at(i), str);
    }
}

}
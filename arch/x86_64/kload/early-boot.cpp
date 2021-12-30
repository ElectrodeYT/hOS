#include <stivale2.h>
#include <early-boot.h>
#include <stdint.h>
#include <stddef.h>
#include <mem/VM/virtmem.h>
#include <mem.h>
#include <CPP/string.h>
#include <stdarg.h>

// Kernel boot stack
static uint8_t stack[4096];

// Terminal callback. We dont actually handle anything either (lol) so just return
void stivale2_terminal_callback(uint64_t type, uint64_t, uint64_t, uint64_t) { (void)type; }

// Terminal function pointer for the terminal we got
void (*stivale2_term_write_ptr)(uint64_t ptr, uint64_t length) = NULL;
// Wrapper to check if its null or not
void stivale2_term_puts(const char* str) {
    // Get string length
    size_t len = 0;
    char* curr = (char*)str;
    while(*curr != '\0') { len++; curr++; }
    stivale2_term_write_ptr((uint64_t)str, len);
}
void stivale2_term_write(const char* fmt, ...) {
    if(!stivale2_term_write_ptr) {
        return;
    }
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
                stivale2_term_puts(char_buf);
                char_buf_ptr = char_buf;
            }
            ptr++;
            switch(*ptr) {
                case '\0': return;
                case 'i':
                case 'd': {
                    char buf[32];
                    itoa(va_arg(args, int), buf, 10);
                    stivale2_term_puts(buf);
                    break;
                }
                case 'x':
                case 'X': {
                    uint64_t i = va_arg(args, uint64_t);
                    stivale2_term_puts("0x");
                    char nibbles[17];
                    nibbles[16] = 0;
                    for(int nibble = 15; nibble >= 0; nibble--) {
                        uint8_t val = (i & ((uint64_t)0xF << (nibble * 4))) >> (nibble * 4);
                        nibbles[15 - nibble] = "0123456789ABCDEF"[val];
                    }
                    stivale2_term_puts(nibbles);
                    break;
                }
                case 's': {
                    stivale2_term_puts(va_arg(args, const char*));
                    break;
                }
                default: continue;
            }
            ptr++;
        } else {
            (*char_buf_ptr++) = *ptr++;
            if((char_buf + char_buf_size) == char_buf_ptr) { char_buf[char_buf_size] = 0; stivale2_term_puts(char_buf); char_buf_ptr = char_buf; }
        }
    }
    if(char_buf != char_buf_ptr) {
        (*char_buf_ptr) = 0;
        stivale2_term_puts(char_buf);
    }
}



void stivale2_term_init(stivale2_struct_tag_terminal* tag) {
    if(!tag) {
        stivale2_term_write_ptr = NULL;
    }
    if(tag->term_write) { 
        stivale2_term_write_ptr = (void (*)(uint64_t, uint64_t))tag->term_write;
    } else {
        stivale2_term_write_ptr = NULL;
    }
}

// Tell stivale2 to give us a terminal function
struct stivale2_header_tag_terminal terminal_hdr_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_TERMINAL_ID,
        .next = 0
    },
    .flags = 1, // we lie here
    .callback = (uintptr_t)stivale2_terminal_callback,
};

// Tell stivale2 to make a framebuffer (definetly not copied)
struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        .next = (uintptr_t)&terminal_hdr_tag
    },
    .framebuffer_width  = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp    = 0,
    .unused = 0
};

// Stivale2 header
struct stivale2_header __attribute__((section(".stivale2hdr"), used)) stivale_hdr = {
    .entry_point = 0,
    .stack = (uintptr_t)stack + sizeof(stack),
    // Enable virtual pointers, PMR, and set deprecated bit 4
    // Disable fully virtual kernel mappings
    .flags = (1 << 1) | (1 << 2) | (1 << 4),
    .tags = (uintptr_t)&framebuffer_hdr_tag
};

// most-certinaly-not-stolen function i definetly should not ever need to not rewrite
void* stivale2_get_tag(struct stivale2_struct *stivale2_struct, uint64_t id) {
    struct stivale2_tag* current_tag = (stivale2_tag*)stivale2_struct->tags;
    for (;;) {
        // If the tag pointer is NULL (end of linked list), we did not find
        // the tag. Return NULL to signal this.
        if (current_tag == NULL) {
            return NULL;
        }

        // Check whether the identifier matches. If it does, return a pointer
        // to the matching tag.
        if (current_tag->identifier == id) {
            return current_tag;
        }

        // Get a pointer to the next tag in the linked list and repeat.
        current_tag = (stivale2_tag*)current_tag->next;
    }
}

// GDT stuff
uint8_t gdt_data[4096] __attribute__((aligned(4 * 1024)));
uint8_t gdt_pointer = 0;

typedef struct {
    uint16_t size;
    uint64_t base;
} __attribute__((packed)) gdt_pointer_t;

gdt_pointer_t gdt_pointer_desc __attribute__((aligned(4 * 1024)));

extern "C" void flush_segments();

// Add descriptor in gdt
void add_gdt_descriptor(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt_data[gdt_pointer++] = limit & 0xFF;
    gdt_data[gdt_pointer++] = (limit >> 8) & 0xFF;
    gdt_data[gdt_pointer++] = base & 0xFF;
    gdt_data[gdt_pointer++] = (base >> 8) & 0xFF;
    gdt_data[gdt_pointer++] = (base >> 16) & 0xFF;
    gdt_data[gdt_pointer++] = access;
    gdt_data[gdt_pointer++] = ((limit >> 16) & 0x0F) | flags;
    gdt_data[gdt_pointer++] = (base >> 24) & 0xFF;
}

// Flush any changes to the gdt
void flush_gdt()  {
   asm volatile("lgdt %0" : : "m" (gdt_pointer_desc));
   flush_segments();
}

// Load the gdt
void load_gdt() {
    add_gdt_descriptor(0, 0, 0, 0);
    add_gdt_descriptor(0, UINT32_MAX, 0b10011010, 0b10100000); // Kernel code
    add_gdt_descriptor(0, UINT32_MAX, 0b10010010, 0b11000000); // Kernel data
    add_gdt_descriptor(0, UINT32_MAX, 0b11111010, 0b10100000); // User code
    add_gdt_descriptor(0, UINT32_MAX, 0b11110010, 0b11000000); // User data

    gdt_pointer_desc.size = sizeof(gdt_data);
    gdt_pointer_desc.base = (uint64_t)gdt_data;
    flush_gdt();
}


// TSS stacks
uint8_t* tss_rsp0;
uint8_t* tss_rsp1;
uint8_t* tss_rsp2;

static unsigned long tss_stack_size = 4; // In pages

struct TSS {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
}__attribute__((packed));

__attribute__((aligned(4096)))
static TSS tss;

/*
uint64_t bits(uint64_t shiftup, uint64_t shiftdown, uint64_t mask, uint64_t val) {
    return ((val >> (shiftdown - mask)) & ((1 << mask) - 1)) << shiftup;
} 
*/

void tss_set_rsp0(uint64_t new_stack_top) {
    tss.rsp0 = new_stack_top;
}

// Load the TSS
// This should be done after virtual memory init
void load_tss() {
    // Allocate the stacks
    // tss_rsp0 = (uint8_t*)Kernel::VM::AllocatePages(tss_stack_size);
    tss_rsp1 = (uint8_t*)Kernel::VM::AllocatePages(tss_stack_size);
    tss_rsp2 = (uint8_t*)Kernel::VM::AllocatePages(tss_stack_size);
    // Zero out the tss
    memset((void*)&tss, 0x00, sizeof(tss));
    
    // Set the interrupt stacks
    // tss.rsp0 = (uint64_t)tss_rsp0 + (tss_stack_size * 4096);
    tss.rsp1 = (uint64_t)tss_rsp1 + (tss_stack_size * 4096);
    tss.rsp2 = (uint64_t)tss_rsp2 + (tss_stack_size * 4096);
    
    // Set iopb to sizeof(tss)
    tss.iopb_offset = 104;

    // We now have to add the TSS descriptor to the GDT.
    // It is so ridicously complicated that we do this manually here.
    // Save current gdt pointer
    int curr_gdt_pointer = gdt_pointer;
    // Advance it twice
    gdt_pointer += 16;
    uint64_t tss_addr = (uint64_t)&tss;

    // Marvel at this absolute stupidity
    gdt_data[curr_gdt_pointer++] = sizeof(TSS) & 0xFF;
    gdt_data[curr_gdt_pointer++] = (sizeof(TSS) >> 8) & 0xFF;
    gdt_data[curr_gdt_pointer++] = (tss_addr >> 0) & 0xFF;
    gdt_data[curr_gdt_pointer++] = (tss_addr >> 8) & 0xFF;
    gdt_data[curr_gdt_pointer++] = (tss_addr >> 16) & 0xFF;
    gdt_data[curr_gdt_pointer++] = 0x89;
    gdt_data[curr_gdt_pointer++] = 0x80;
    gdt_data[curr_gdt_pointer++] = (tss_addr >> 24) & 0xFF;
    gdt_data[curr_gdt_pointer++] = (tss_addr >> 32) & 0xFF;
    gdt_data[curr_gdt_pointer++] = (tss_addr >> 40) & 0xFF;
    gdt_data[curr_gdt_pointer++] = (tss_addr >> 48) & 0xFF;
    gdt_data[curr_gdt_pointer++] = (tss_addr >> 56) & 0xFF;
    gdt_data[curr_gdt_pointer++] = 0;
    gdt_data[curr_gdt_pointer++] = 0;
    gdt_data[curr_gdt_pointer++] = 0;
    gdt_data[curr_gdt_pointer++] = 0;
 /*

    uint64_t* gdt_longs = (uint64_t*)gdt_data;
    size_t gdt_long_pointer = curr_gdt_pointer / 8;
    gdt_longs[gdt_long_pointer] = bits(16, 24, 24, tss_addr) | bits(56, 32, 8, tss_addr) | (103 & 0xff) | (0b1001UL << 40) | (1UL << 47);
    gdt_longs[gdt_long_pointer + 1] = tss_addr >> 32;
    */flush_gdt();
    asm volatile("ltr %w0" : : "qm"((gdt_pointer - 16)));
}
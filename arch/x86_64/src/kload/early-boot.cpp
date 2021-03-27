#include <stivale2.h>
#include <early-boot.h>
#include <stdint.h>
#include <stddef.h>
#include <mem/virtmem.h>
#include <mem.h>

// Kernel boot stack
static uint8_t stack[4096];

// Tell stivale2 to make a framebuffer (definetly not copied)
struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
    // All tags need to begin with an identifier and a pointer to the next tag.
    .tag = {
        // Identification constant defined in stivale2.h and the specification.
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        // If next is 0, then this marks the end of the linked list of tags.
        .next = 0
    },
    // We set all the framebuffer specifics to 0 as we want the bootloader
    // to pick the best it can.
    .framebuffer_width  = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp    = 0
};

// Stivale2 header
struct stivale2_header __attribute__((section(".stivale2hdr"), used)) stivale_hdr = {
    // The entry_point member is used to specify an alternative entry
    // point that the bootloader should jump to instead of the executable's
    // ELF entry point. We do not care about that so we leave it zeroed.
    .entry_point = 0,
    // Let's tell the bootloader where our stack is.
    // We need to add the sizeof(stack) since in x86(_64) the stack grows
    // downwards.
    .stack = (uintptr_t)stack + sizeof(stack),
    // No flags are currently defined as per spec and should be left to 0.
    .flags = 0,
    // This header structure is the root of the linked list of header tags and
    // points to the first one (and in our case, only).
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
// Load the TSS
// This should be done after virtual memory init
void load_tss() {
    // Allocate the stacks
    tss_rsp0 = (uint8_t*)Kernel::VirtualMemory::the().AllocatePages(tss_stack_size);
    tss_rsp1 = (uint8_t*)Kernel::VirtualMemory::the().AllocatePages(tss_stack_size);
    tss_rsp2 = (uint8_t*)Kernel::VirtualMemory::the().AllocatePages(tss_stack_size);
    // Zero out the tss
    memset((void*)&tss, 0x00, sizeof(tss));
    
    // Set the interrupt stacks
    tss.rsp0 = (uint64_t)tss_rsp0 + (tss_stack_size * 4096);
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
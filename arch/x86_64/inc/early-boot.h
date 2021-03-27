#ifndef EARLYBOOT_H
#define EARLYBOOT_H
void *stivale2_get_tag(struct stivale2_struct *stivale2_struct, uint64_t id);
void load_gdt();
void load_tss();
void flush_gdt();

#endif
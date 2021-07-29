#ifndef EARLYBOOT_H
#define EARLYBOOT_H
void *stivale2_get_tag(struct stivale2_struct *stivale2_struct, uint64_t id);
void load_gdt();
void load_tss();
void flush_gdt();
void tss_set_rsp0(uint64_t new_stack_top);

#endif
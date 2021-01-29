#include <memory/memorymanager.h>
#include <memory/heap.h>
#include <multiboot/multiboot.h>
#include <debug/debug_print.h>

namespace Kernel {
    namespace MemoryManager {
        namespace PageFrameAllocator {
            // Linked list of memory
            struct PhysMemSegmentLinkedList {
                PhysMemSegment* seg;
                PhysMemSegmentLinkedList* next = nullptr;
            };

            PhysMemSegmentLinkedList* linked_list = nullptr;
            
            int InitPageFrameAllocator(void* multiboot_info) {
                // We want to loop through the mmap
                volatile multiboot_info_t* multiboot = (volatile multiboot_info_t*)multiboot_info;
                // Halt if the multiboot info does not have memory map info
                if(!multiboot->flags & MULTIBOOT_INFO_MEM_MAP) { debug_puts("Multiboot info does not have memory map! Halting.\n\r"); while(1); }
                for(int i = 0; i < multiboot->mmap_length; i += sizeof(multiboot_memory_map_t)) {
                    // Get entry
                    // mmap_adr is a virtual address, so we need to add the kernel virtual base to it.
                    multiboot_memory_map_t* entry = (multiboot_memory_map_t*)(multiboot->mmap_addr + KernelVirtualBase + i);
                    // Check if we can use this segment
                    if(entry->addr >> 32) { debug_puts("Skipping entry "); debug_puti(i); debug_puts(" in the mmap table, as it has a PAE address.\n\r"); continue; }
                    // Check if its a null entry (sometimes happens)
                    if(entry->addr == 0 && entry->len == 0) { debug_puts("Skipping Null entry"); continue; }
                    // Create physmemsegment object
                    PhysMemSegment* segment = new PhysMemSegment;
                    segment->adr = entry->addr;
                    segment->len = entry->len;
                    segment->type = entry->type;

                    // If this is allocatable memory, then we need to create the page allocation bitmap for it
                    if(segment->type == MULTIBOOT_MEMORY_AVAILABLE) {
                        uint32_t page_count = segment->len / (4 * 1024);
                        segment->page_bitmap = new uint8_t[page_count / 8]; // Divide by 8 as its a bitmap
                    }

                    // Create linked list object
                    PhysMemSegmentLinkedList* obj = new PhysMemSegmentLinkedList;
                    obj->seg = segment;

                    // Get the last entry in the linked list
                    PhysMemSegmentLinkedList* ll = linked_list;
                    if(ll == nullptr) {
                        // First entry, just set linked_list = obj;
                        linked_list = obj;
                        continue;
                    }
                    while(ll->next != nullptr) {
                        ll = ll->next;
                    }
                    ll->next = obj;
                }
                debug_puts("Survived page frame allocation\n\r");
                return 0;
            }

            void* AllocatePage() {
                // Current selected memory segment
                PhysMemSegmentLinkedList* curr = linked_list;
                while(curr != nullptr) {
                    // Check if its a normal memory page
                    if(curr->seg->type != MULTIBOOT_MEMORY_AVAILABLE) { curr = curr->next; continue; }
                    // This page has available memory, check if theres any free
                    // To improve speed, we first perform a 32 bit check to see if any bits are free
                    int bitmap_length = (curr->seg->len / (4 * 1024)) / 8;
                    for(uint32_t i = 0; i < bitmap_length; i++) {
                        if(curr->seg->page_bitmap[i] != ~((uint8_t)0)) {
                            // There is a free page here, allocate it
                            for(int x = 0; x < 7; x++) {
                                if((curr->seg->page_bitmap[i] >> x) & 1) {
                                    // This page is free, calculate its in-segment page offset
                                    int page_offset = (i * 8) + x;
                                    // Now calculate its address
                                    int page_address = curr->seg->adr + (page_offset * 4096);
                                    // Set the bit allocating it
                                    curr->seg->page_bitmap[i] |= (1 << x);
                                    // Return the page
                                    return (void*)page_address;
                                }
                            }
                        }
                    }
                    // No memory was available in this segment, go to the next one
                    curr = curr->next;
                }

                // No memory available
                return nullptr;
            }

            // TODO: Free page
            int FreePage(void* page) {
                debug_puts("TODO: FreePage()\n\r");
            }
        }
    }
}
#ifndef PHYSALLOC_H
#define PHYSALLOC_H

#include <stddef.h>
#include <stdint.h>

struct physmem_ll {
    // Base address
    uint64_t base;
    // Size of this segment
    size_t size;
    // Types of physical memory
    #define PHYSMEM_LL_TYPE_USEABLE 0       // Usable memory
    #define PHYSMEM_LL_TYPE_RESERVED 1      // Memory reserved for a reason not available
    #define PHYSMEM_LL_TYPE_ACPI_REC 2      // Reclaimable with ACPI
    #define PHYSMEM_LL_TYPE_ACPI_NVS 3      // ACPI NVS
    #define PHYSMEM_LL_TYPE_BAD 4           // Existing, but bad memory
    #define PHYSMEM_LL_TYPE_KERNEL 5        // Kernel location
    #define PHYSMEM_LL_TYPE_BOOTLOADER 6    // Bootloader data
    uint8_t type;
    // Allocation bitmap
    uint8_t* bitmap;
    // Next entry in the list
    physmem_ll* next;
} __attribute__((packed));

void __init_physical_allocator(physmem_ll* memory_map);

#endif
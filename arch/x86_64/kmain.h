#ifndef KMAIN_H
#define KMAIN_H

namespace Kernel {
    void KernelMain(size_t mod_count, uint8_t** modules, uint64_t* module_sizes, char** module_names);
}

#endif
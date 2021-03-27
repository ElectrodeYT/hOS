#ifndef PROCESS_H
#define PROCESS_H

namespace Kernel {
    namespace Processes {
        struct Process {
            Hardware::Registers regs;
            uint64_t page;
        };
    }
}

#endif
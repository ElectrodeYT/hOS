#ifndef SERIAL_H
#define SERIAL_H

void __init_serial();

namespace Kernel {
    namespace Debug {
        void SerialPrint(const char* c);
        void SerialPrintInt(long i);
    }
}

#endif
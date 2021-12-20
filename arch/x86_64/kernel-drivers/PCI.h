 
#ifndef PCI_H
#define PCI_H

#include <stdint.h>

namespace Kernel {
    class PCI {
    public:
        static PCI& the() {
            static PCI instance;
            return instance;
        }

        bool probe();


        const char* classToString(uint8_t c);
        const char* subclassToString(uint8_t c, uint8_t s);
    private:

        uint16_t configRead(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
        inline uint16_t deviceVendor(uint8_t bus, uint8_t slot, uint8_t function) { return configRead(bus, slot, function, 0); }
        inline uint16_t deviceID(uint8_t bus, uint8_t slot, uint8_t function) { return configRead(bus, slot, function, 0x2); }
        inline uint16_t deviceHeader(uint8_t bus, uint8_t slot, uint8_t function) { return configRead(bus, slot, function, 0xE); }
        inline uint8_t deviceClass(uint8_t bus, uint8_t slot, uint8_t function) { return ((configRead(bus, slot, function, 0xA) >> 8) & 0xFF); }
        inline uint8_t deviceSubClass(uint8_t bus, uint8_t slot, uint8_t function) { return (configRead(bus, slot, function, 0xA) & 0xFF); }
        
        void probeDevice(uint8_t bus, uint8_t slot);
        void probeDeviceFunction(uint8_t bus, uint8_t slot, uint8_t function);
    };
}

#endif

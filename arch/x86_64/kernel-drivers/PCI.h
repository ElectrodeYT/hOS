 
#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <CPP/mutex.h>
#include <CPP/vector.h>

namespace Kernel {

class PCIDevice;

class PCI {
public:
    static PCI& the() {
        static PCI instance;
        return instance;
    }

    bool probe();

    const char* classToString(uint8_t c);
    const char* subclassToString(uint8_t c, uint8_t s);

    uint16_t configRead(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) { acquire(&mutex); uint16_t ret = configReadImpl(bus, slot, func, offset); release(&mutex); return ret; }
    void configWrite(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data);
    
    inline uint16_t deviceVendor(uint8_t bus, uint8_t slot, uint8_t function) { return configRead(bus, slot, function, 0); }
    inline uint16_t deviceID(uint8_t bus, uint8_t slot, uint8_t function) { return configRead(bus, slot, function, 0x2); }
    inline uint16_t deviceHeader(uint8_t bus, uint8_t slot, uint8_t function) { return configRead(bus, slot, function, 0xE); }
    inline uint8_t deviceClass(uint8_t bus, uint8_t slot, uint8_t function) { return ((configRead(bus, slot, function, 0xA) >> 8) & 0xFF); }
    inline uint8_t deviceSubClass(uint8_t bus, uint8_t slot, uint8_t function) { return (configRead(bus, slot, function, 0xA) & 0xFF); }
    inline uint8_t deviceProgIF(uint8_t bus, uint8_t slot, uint8_t function) { return (configRead(bus, slot, function, 0x8) >> 8) & 0xFF; }

    inline void deviceWriteProgIF(uint8_t bus, uint8_t slot, uint8_t function, uint8_t val) {
        uint16_t tmp = (configRead(bus, slot, function, 0x8) & 0x00FF) | (val << 8);
        configWrite(bus, slot, function, 0x8, tmp);
    }
private:
    mutex_t mutex;

    uint16_t configReadImpl(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    void probeDevice(uint8_t bus, uint8_t slot);
    void probeDeviceFunction(uint8_t bus, uint8_t slot, uint8_t function);

    Vector<PCIDevice*> device_drivers;
};

class PCIDevice {
public:
    PCIDevice(uint8_t _bus, uint8_t _slot, uint8_t _function) : bus(_bus), slot(_slot), function(_function) {
        header_type = PCI::the().deviceHeader(bus, slot, function);
        if(header_type == 0x0 || header_type == 0x1) {
            // BAR0,1 exists
            bar0 = (PCI::the().configRead(bus, slot, function, 0x12) << 16) | PCI::the().configRead(bus, slot, function, 0x10);
            bar1 = (PCI::the().configRead(bus, slot, function, 0x16) << 16) | PCI::the().configRead(bus, slot, function, 0x14);
        }
        if(header_type == 0x0) {
            // BAR2-5 exists
            bar2 = (PCI::the().configRead(bus, slot, function, 0x1A) << 16) | PCI::the().configRead(bus, slot, function, 0x18);
            bar3 = (PCI::the().configRead(bus, slot, function, 0x1E) << 16) | PCI::the().configRead(bus, slot, function, 0x1C);
            bar4 = (PCI::the().configRead(bus, slot, function, 0x22) << 16) | PCI::the().configRead(bus, slot, function, 0x20);
            bar5 = (PCI::the().configRead(bus, slot, function, 0x26) << 16) | PCI::the().configRead(bus, slot, function, 0x24);
        }
    }
    virtual ~PCIDevice() { }

    virtual bool Initialize() { return false; }
    virtual void KernelTask(void* arg) { for(;;); (void)arg; }
    virtual bool hasKernelTask() { return false; }
protected:
    uint8_t bus;
    uint8_t slot;
    uint8_t function;

    uint8_t header_type = 0;
    uint32_t bar0 = 0;
    uint32_t bar1 = 0;
    uint32_t bar2 = 0;
    uint32_t bar3 = 0;
    uint32_t bar4 = 0;
    uint32_t bar5 = 0;
};

}

#endif

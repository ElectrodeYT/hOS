#include <mem.h>
#include <kernel-drivers/PCI.h>
#include <hardware/instructions.h>
#include <debug/serial.h>

namespace Kernel {
    uint16_t PCI::configRead(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
        uint32_t address;
        uint32_t lbus  = (uint32_t)bus;
        uint32_t lslot = (uint32_t)slot;
        uint32_t lfunc = (uint32_t)func;
        uint16_t tmp = 0;

        // Create configuration address as per Figure 1
        address = (uint32_t)((lbus << 16) | (lslot << 11) |
                    (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

        // Write out the address
        outl(0xCF8, address);
        // Read in the data
        // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
        tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
        return tmp;
    }

    bool PCI::probe() {
        Debug::SerialPrintf("--- BEGIN PCI PROBE ---\n\r");
        for(uint16_t bus = 0; bus < 256; bus++) { // Bus is actually 8 bits, but since we count from 0 -> 255 and i cant be bothered to make a non-forever looping version of that this will work
            for(uint8_t device = 0; device < 32; device++) {
                probeDevice((uint8_t)bus, device);
            }
        }
        Debug::SerialPrintf("--- END PCI PROBE ---\n\r");
        return true;
    }

    void PCI::probeDevice(uint8_t bus, uint8_t slot) {
        // Check whether this device actually exists
        if(deviceVendor(bus, slot, 0) == 0xFFFF) { return; }
        probeDeviceFunction(bus, slot, 0); // Function 0 always exists
        if(deviceHeader(bus, slot, 0)) {
            // Multi function device, probe individual functions
            for(size_t i = 1; i < 8; i++) { probeDeviceFunction(bus, slot, i); }
        }
    }
    
    void PCI::probeDeviceFunction(uint8_t bus, uint8_t slot, uint8_t function) {
        if(deviceVendor(bus, slot, function) == 0xFFFF) { return; } // Doesnt exist
        uint8_t device_class = configRead(bus, slot, function, 0xA);
        uint8_t device_subclass = configRead(bus, slot, function, 0xB);
        Debug::SerialPrintf("%i:%i.%i: %s; %s\n\r", (unsigned int)bus, (unsigned int)slot, (unsigned int)function, classToString(device_class), subclassToString(device_class, device_subclass));
    }

    const char* PCI::classToString(uint8_t c) {
        switch(c) {
            case 0x0: return "Unclassified";
            case 0x1: return "Mass Storage Controller";
            case 0x2: return "Network Controller";
            case 0x3: return "Display Controller";
            case 0x4: return "Multimedia Controller";
            case 0x5: return "Memory Controller";
            case 0x6: return "Bridge";
            case 0x7: return "Simple Communication Controller";
            case 0x8: return "Base System Peripheral";
            case 0x9: return "Input Device Controller";
            case 0xA: return "Docking Station";
            case 0xB: return "Processor";
            case 0xC: return "Serial Bus Controller";
            case 0xD: return "Wireless Controller";
            case 0xE: return "Intelligent Controller";
            case 0xF: return "Satellite Communication Controller";
            case 0x10: return "Encryption Controller";
            case 0x11: return "Signal Processing Controller";
            case 0x12: return "Processing Accelerator";
            case 0x13: return "Non-Essential Instrumentation";
            case 0x40: return "Co-Processor";
            case 0xFF: return "Vendor-Specific";
            default: return "Unknown";
        }
    }

    const char* PCI::subclassToString(uint8_t c, uint8_t s) {
        if(c == 0x0) {
            switch(s) {
                case 0: return "Non-VGA Compatible Unclassified Device";
                case 1: return "VGA Compatible Unclassified Device";
            }
        } else if(c == 0x1) {
            switch(s) {
                case 0: return "SCSI Bus Controller";
                case 1: return "IDE Controller";
                case 2: return "Floppy Disk Controller";
                case 3: return "IPI Bus Controller";
                case 4: return "RAID Controller";
                case 5: return "ATA Controller";
                case 6: return "Serial ATA Controller";
                case 7: return "Serial Attached SCSI Controller";
                case 8: return "Non-Volatile Memory Controller";
                case 0x80: return "Other Mass Storage Controller";
            }
        }
        return "Unknown";
    }
}

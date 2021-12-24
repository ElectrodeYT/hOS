#ifndef IDE_H
#define IDE_H
#include <hardware/instructions.h>
#include <kernel-drivers/PCI.h>
#include <kernel-drivers/BlockDevices.h>
#include <CPP/mutex.h>
#include <interrupts.h>

namespace Kernel {

class IDEBlockDevice;

class IDEDevice : public PCIDevice {
public:
    IDEDevice(uint8_t _bus, uint8_t _slot, uint8_t _function) : PCIDevice(_bus, _slot, _function) { }
    bool Initialize() override;
    bool hasKernelTask() override { return false; }

    bool InitPIO();

    int read(int id, void* buf, size_t len, size_t offset);
    int write(int id, void* buf, size_t len, size_t offset);

    void IRQ() { waiting_on_irq = false;}
private:
    
    bool waiting_on_irq = false;

    bool Detect(IDEBlockDevice* device);

    inline void SendCommand(uint8_t cmd) {
        if(is_secondary) {
            outb(secondary_bus_io_base + 7, cmd);
        } else {
            outb(primary_bus_io_base + 7, cmd);
        }
    }

    inline void WriteLBA(uint64_t lba, uint16_t sec_count) {
        if(is_secondary) {
            outb(secondary_bus_io_base + 2, (sec_count >> 8) & 0xFF); // Sec high
            outb(secondary_bus_io_base + 3, (lba >> 24) & 0xFF); // LBA4
            outb(secondary_bus_io_base + 4, (lba >> 32) & 0xFF); // LBA5
            outb(secondary_bus_io_base + 5, (lba >> 40) & 0xFF); // LBA6
            outb(secondary_bus_io_base + 2, (sec_count >> 8) & 0xFF); // Sec low
            outb(secondary_bus_io_base + 3, (lba >> 0) & 0xFF); // LBA1
            outb(secondary_bus_io_base + 4, (lba >> 8) & 0xFF); // LBA2
            outb(secondary_bus_io_base + 5, (lba >> 16) & 0xFF); // LBA3
        } else {
            outb(primary_bus_io_base + 2, (sec_count >> 8) & 0xFF); // Sec high
            outb(primary_bus_io_base + 3, (lba >> 24) & 0xFF); // LBA4
            outb(primary_bus_io_base + 4, (lba >> 32) & 0xFF); // LBA5
            outb(primary_bus_io_base + 5, (lba >> 40) & 0xFF); // LBA6
            outb(primary_bus_io_base + 2, (sec_count >> 8) & 0xFF); // Sec low
            outb(primary_bus_io_base + 3, (lba >> 0) & 0xFF); // LBA1
            outb(primary_bus_io_base + 4, (lba >> 8) & 0xFF); // LBA2
            outb(primary_bus_io_base + 5, (lba >> 16) & 0xFF); // LBA3
        }
    }

    inline uint8_t ReadStatus() {
        if(is_secondary) {
            return inb(secondary_bus_io_base + 7);
        } else {
            return inb(primary_bus_io_base + 7);
        }
    }

    inline uint8_t ReadError() {
        if(is_secondary) {
            return inb(secondary_bus_io_base + 1);
        } else {
            return inb(primary_bus_io_base + 1);
        }
    }

    inline uint16_t ReadDataShortFromPri() { return inw(primary_bus_io_base); }
    inline uint16_t ReadDataShortFromSec() { return inw(secondary_bus_io_base); }

    inline void DriveSelect(bool slave, bool _is_secondary) {
        is_secondary = _is_secondary;
        int make_sure_slave_is_a_one = slave ? 1 : 0;
        if(is_secondary) {
            outb(secondary_bus_io_base + 6, 0xE0 | (make_sure_slave_is_a_one << 4));
        } else {
            outb(primary_bus_io_base + 6, 0xE0 | (make_sure_slave_is_a_one << 4));
        }
    }

    inline bool PollUntilReady() {
        // TODO: timeout
        while(ReadStatus() & 0x80);
        return true;
    }

    bool is_secondary = false;
    mutex_t mutex;

    enum StatusPortBitmask {
        ATA_SR_ERR = 0x1, // Error
        ATA_SR_IDX = 0x2, // Index
        ATA_SR_CORR = 0x4, // Corrected data
        ATA_SR_DRQ = 0x8, // Data Request Ready
        ATA_SR_DSC = 0x10, // Drive Seek Complete
        ATA_SR_DF = 0x20, // Drive Write Fault
        ATA_SR_DRDY = 0x40, // Drive Read
        ATA_SR_BSY = 0x80 // Drive Busy
    };

    enum ErrorPortBitmask {
        ATA_ER_AMNF = 0x1, // No Address Mark
        ATA_ER_TK0NF = 0x2, // Track 0 Not Found
        ATA_ER_ABRT = 0x4, // Command Aborted
        ATA_ER_MCR = 0x8, // Media Change Request
        ATA_ER_IDNF = 0x10, // ID Mark Not Found
        ATA_ER_MC = 0x20, // Media Changed
        ATA_ER_UNC = 0x40, // Uncorrectable Data
        ATA_ER_BBK = 0x80 // Bad Block
    };

    enum CommandPortBitmask {
        ATA_CMD_READ_PIO = 0x20,
        ATA_CMD_READ_PIO_EXT = 0x24,
        ATA_CMD_READ_DMA = 0xC8,
        ATA_CMD_READ_DMA_EXT = 0x25,
        ATA_CMD_WRITE_PIO = 0x30,
        ATA_CMD_WRITE_PIO_EXT = 0x34,
        ATA_CMD_WRITE_DMA = 0xCA,
        ATA_CMD_WRITE_DMA_EXT = 0x35,
        ATA_CMD_CACHE_FLUSH = 0xE7,
        ATA_CMD_CACHE_FLUSH_EXT = 0xEA,
        ATA_CMD_PACKET = 0xA0,
        ATA_CMD_IDENTIFY_PACKET = 0xA1,
        ATA_CMD_IDENTIFY = 0xEC
    };

    // Standard PIO pin bases
    const uint16_t primary_bus_io_base = 0x1F0;
    const uint16_t primary_bus_control_base = 0x3F6;
    const uint16_t secondary_bus_io_base = 0x170;
    const uint16_t secondary_bus_control_base = 0x376;

    IDEBlockDevice* devices;
};

struct IDEBlockDevice : public BlockDevice {
    IDEBlockDevice() : BlockDevice() { }
    IDEDevice* driver;
    int id;
    bool exists = false;
    int read(void* buf, size_t len, size_t offset) override { return driver->read(id, buf, len, offset); }
    int write(void* buf, size_t len, size_t offset) override { return driver->write(id, buf, len, offset); }
};

void IDECompatPriInterrupt(Interrupts::ISRRegisters* regs);
void IDECompatSecInterrupt(Interrupts::ISRRegisters* regs);


}

#endif
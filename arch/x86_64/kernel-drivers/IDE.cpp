#include <kernel-drivers/IDE.h>
#include <processes/scheduler.h>
#include <debug/klog.h>
#include <panic.h>
#include <errno.h>
#include <mem.h>

namespace Kernel {

IDEDevice* device_holding_irq = NULL;

bool IDEDevice::Initialize() {
    KLog::the().printf("IDE: initializing IDE controller at %i:%i.%i\n\r", bus, slot, function);
    // Check which type of controller this is
    devices = new IDEBlockDevice[4];
    devices[0].id = 0;
    devices[0].driver = this;
    devices[1].id = 1;
    devices[1].driver = this;
    devices[2].id = 2;
    devices[2].driver = this;
    devices[3].id = 3;
    devices[3].driver = this;

    uint8_t prog_if = PCI::the().deviceProgIF(bus, slot, function);
    bool success = false;
    if(prog_if & 0x2) {
        KLog::the().printf("IDE: controller supports PCI-Native and IDE-Compat mode\n\r");
        success = InitPIO();
    } else if(prog_if & 0x1) {
        KLog::the().printf("IDE: controller is PCI-Native only\n\r");
        success = false;
    } else {
        KLog::the().printf("IDE: controller is IDE-Compat only\n\r");
        success = InitPIO();
    }


    
    return success;
}

bool IDEDevice::InitPIO() {
    // Write 0 to the ProgIF bit
    // This will force PCI/IDE mode controllers to IDE-Compat mode,
    // and should have no effect on just IDE-Compat mode controllers
    uint8_t prog_if = PCI::the().deviceProgIF(bus, slot, function) & ~(1);
    PCI::the().deviceWriteProgIF(bus, slot, function, prog_if);

    // We now check each drive by polling each drive
    // 1. Channel master
    DriveSelect(false, false);
    if(Detect(&(devices[0]))) {
        KLog::the().printf("IDE: Drive 0 is attached\n\r");
        BlockManager::the().RegisterBlockDevice(&(devices[0]));
    }
    // 1. Channel slave
    DriveSelect(true, false);
    if(Detect(&(devices[1]))) {
        KLog::the().printf("IDE: Drive 1 is attached\n\r");
        BlockManager::the().RegisterBlockDevice(&(devices[1]));
    }
    // 2. Channel master
    DriveSelect(false, true);
    if(Detect(&(devices[2]))) {
        KLog::the().printf("IDE: Drive 2 is attached\n\r");
        BlockManager::the().RegisterBlockDevice(&(devices[2]));
    }
    // 2. Channel slave
    DriveSelect(true, true);
    if(Detect(&(devices[3]))) {
        KLog::the().printf("IDE: Drive 3 is attached\n\r");
        BlockManager::the().RegisterBlockDevice(&(devices[3]));
    }
    // Steal the IRQs
    Interrupts::the().RegisterIRQHandler(14, IDECompatPriInterrupt);
    Interrupts::the().RegisterIRQHandler(15, IDECompatSecInterrupt);
    device_holding_irq = this;

    return true;
}

bool IDEDevice::Detect(IDEBlockDevice* device) {
    WriteLBA(0, 0);
    SendCommand(ATA_CMD_IDENTIFY);
    if(ReadStatus() == 0) { return false; }
    // We need to poll until the busy bit clears
    while(ReadStatus() & 0x80);
    // Check the LBAmid and LBAhigh ports
    bool non_zero = false;
    if(is_secondary) {
        if(inb(secondary_bus_io_base + 4)) non_zero = true;
        if(inb(secondary_bus_io_base + 5)) non_zero = true;
    } else {
        if(inb(primary_bus_io_base + 4)) non_zero = true;
        if(inb(primary_bus_io_base + 5)) non_zero = true;
    }
    if(non_zero) {
        KLog::the().printf("IDE: Device is ATAPI, treating as not attached\n\r");
        return false;
    }
    // We can be sure that the drive will (at some point) assert DRQ or ERR
    // Providing that the drive did not error, read the ident info
    while(true) {
        // TODO: timeout
        uint8_t status = ReadStatus();
        if(status & ATA_SR_ERR) { KLog::the().printf("IDE: Device errored during IDENT\n\r"); return false; }
        if(status & ATA_SR_DRQ) { break; }
    }

    // Time to read 256 uint16_ts
    uint16_t ident_info[256];
    if(is_secondary) {
        for(size_t i = 0; i < 256; i++) { ident_info[i] = ReadDataShortFromSec(); }
    } else {
        for(size_t i = 0; i < 256; i++) { ident_info[i] = ReadDataShortFromPri(); }
    }

    // We only support LBA48 lol
    if(!(ident_info[83] & (1 << 10))) { KLog::the().printf("IDE: Device does not support LBA48\n\r"); }

    // Get the LBA sector count
    uint64_t lba_sector_count = 0;
    lba_sector_count |= ident_info[100];
    lba_sector_count |= ((uint64_t)(ident_info[101]) << 16);
    lba_sector_count |= ((uint64_t)(ident_info[102]) << 32);
    lba_sector_count |= ((uint64_t)(ident_info[103]) << 48);
    KLog::the().printf("IDE: Device sector count: %x\n\r", lba_sector_count);
    KLog::the().printf("IDE: Device byte count: %x\n\r", lba_sector_count * 512);
    device->len = lba_sector_count * 512;
    device->exists = true;
    return true;
}

int IDEDevice::read(int id, void* buf, size_t len, size_t offset) {
    if(!devices[id].exists) { return -ENODEV; }
    if(len >= devices[id].len) { return 0; }
    ASSERT(len, "IDEDevice: read with 0 length");
    // Constrain len
    if((len + offset) >= devices[id].len) {
        len = devices[id].len - offset;
    }
    acquire(&mutex);
    switch(id) {
        case 0: DriveSelect(false, false); break;
        case 1: DriveSelect(true, false); break;
        case 2: DriveSelect(false, true); break;
        case 3: DriveSelect(true, true); break;
    }
    // Read the status register to waste time
    ReadStatus();
    // Calculate sector(s)
    uint64_t sector = offset / 512;
    uint64_t last_sector = (offset + len - 1) / 512;
    uint64_t sector_count = (last_sector - sector) + 1;
    // Get the sectors
    // First we need to calculate the offset in the first sector
    uint64_t first_sector_offset = offset % 512;
    int64_t len_to_go = (int64_t)len;
    // Allocate memory for the sectors
    // TODO: maybe allocate this using VM::?
    uint16_t* buffer;
    if((sector_count * 512) > 4096) {
        buffer = (uint16_t*)VM::AllocatePages(((sector_count * 512) / 4096) + (((sector_count * 512) & 4095) ? 1 : 0));
    } else {
        buffer = new uint16_t[sector_count * 256];
    }
    uint16_t* curr = buffer;
    // Send to command
    waiting_on_irq = true;
    WriteLBA(sector, sector_count);
    SendCommand(ATA_CMD_READ_PIO_EXT);
    // Check if we are already ready lol
    //if(!(ReadStatus() & ATA_SR_DRQ)) {
    //    while(waiting_on_irq);
    //}
    while(len_to_go > 0) {
        while(!(ReadStatus() & ATA_SR_DRQ));
        // The data is ready, time to roll
        if(is_secondary) {
            for(size_t i = 0; i < 256; i++) { *(curr++) = ReadDataShortFromSec(); }
        } else {
            for(size_t i = 0; i < 256; i++) { *(curr++) = ReadDataShortFromPri(); }
        }
        len_to_go -= 512;
    }
    ASSERT(!(ReadStatus() & ATA_SR_DRQ), "IDE: Drive DRQ asserted after read done");
    // Memcopy the data we want
    memcopy(((uint8_t*)(buffer)) + first_sector_offset, buf, len);
    if((sector_count * 512) > 4096) {
        VM::FreePages(buffer, ((sector_count * 512) / 4096) + (((sector_count * 512) & 4095) ? 1 : 0));
    } else {
        delete buffer;
    }
    release(&mutex);
    return len;
}

int IDEDevice::write(int id, void* buf, size_t len, size_t offset) {
    return -ENOSYS;
    (void)id;
    (void)buf;
    (void)len;
    (void)offset;
}

void IDECompatPriInterrupt(Interrupts::ISRRegisters* regs) {
    if(device_holding_irq) {
        device_holding_irq->IRQ();
    }
    (void)regs;
}

void IDECompatSecInterrupt(Interrupts::ISRRegisters* regs) {
    if(device_holding_irq) {
        device_holding_irq->IRQ();
    }
    (void)regs;
}

}
#include <kernel-drivers/BlockDevices.h>
#include <debug/klog.h>

namespace Kernel {

int BlockManager::RegisterBlockDevice(BlockDevice* device) {
    device->block_device_id = next_id++;
    BlockDeviceContainer* container = new BlockDeviceContainer;
    container->main = device;
    devices.push_back(container);
    return device->block_device_id;
}

BlockDevice* BlockManager::GetBlockDevice(int block_id, int part_id) {
    for(size_t i = 0; i < devices.size(); i++) {
        if(part_id == 0 && devices.at(i)->main->block_device_id == block_id) {
            return devices.at(i)->main;
        } else if(part_id > 0) {
            if((size_t)(part_id - 1) >= devices.at(i)->partitions.size()) { return NULL; }
            return devices.at(i)->partitions.at(part_id - 1);
        }
    }
    return NULL;
}

void BlockManager::ParsePartitions() {
    for(size_t dev = 0; dev < devices.size(); dev++) {
        BlockDevice* device = devices.at(dev)->main;
        // Check if there even is a GPT partition table here
        uint64_t* lba1 = new uint64_t[512 / 8];
        device->read(lba1, 512, 512);
        const uint8_t efi_magic[8] = {0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54};
        if(*lba1 != *((uint64_t*)efi_magic)) {
            KLog::the().printf("BlockManager: device %i does not have a GPT header\n\r", dev);
            continue;
        }
        // We do have a GPT header, parse it
        // Get max partition count
        uint32_t partition_count = *(uint32_t*)((uint8_t*)(lba1) + 0x50);
        // Get LBA of first partition header
        uint64_t current_read_offset = *(uint64_t*)((uint8_t*)(lba1) + 0x48) * 512;
        // Get size of each partition header
        uint64_t partition_header_size = *(uint32_t*)((uint8_t*)(lba1) + 0x54) * 512;
        
        uint64_t* header = new uint64_t[partition_header_size];
        uint64_t real_part_count = 0;
        for(size_t part = 0; part < partition_count; part++) {
            device->read(header, partition_header_size, current_read_offset);
            current_read_offset += partition_header_size;
            // Check if this is a NULL partition
            if(!(*header) && !(*(header + 1))) { continue; }
            // This is a actual partition, create a device for it
            PartitionBlockDevice* part_device = new PartitionBlockDevice;
            part_device->main = device;
            part_device->offset = *(header + 4) * 512;
            part_device->len = (*(header + 5) * 512) - part_device->offset;
            part_device->guid[0] = *(header + 2);
            part_device->guid[1] = *(header + 3);  
            devices.at(dev)->partitions.push_back((BlockDevice*)part_device);
            KLog::the().printf("BlockManager: created parttion %i device for device %i, offset %x, size %x\n\r", real_part_count++, dev, part_device->offset, part_device->len);
        }
        delete header;
    }
}

int PartitionBlockDevice::read(void* buf, size_t len, size_t _offset) {
    // TODO: constrain partition size
    return main->read(buf, len, offset + _offset);
}

int PartitionBlockDevice::write(void* buf, size_t len, size_t _offset) {
    // TODO: constrain partition size
    return main->write(buf, len, offset + _offset);
}


}
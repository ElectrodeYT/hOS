#include <kernel-drivers/BlockDevices.h>

namespace Kernel {

int BlockManager::RegisterBlockDevice(BlockDevice* device) {
    device->block_device_id = next_id++;
    devices.push_back(device);
    return device->block_device_id;
}

BlockDevice* BlockManager::GetBlockDevice(int block_id) {
    for(size_t i = 0; i < devices.size(); i++) {
        if(devices.at(i)->block_device_id == block_id) {
            return devices.at(i);
        }
    }
    return NULL;
}

}
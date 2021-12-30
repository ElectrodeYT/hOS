#ifndef BLOCKDEVICES_H
#define BLOCKDEVICES_H
#include <stddef.h>
#include <stdint.h>
#include <CPP/vector.h>
#include <errno.h>

namespace Kernel {

struct BlockDevice {
    virtual int read(void* buf, size_t len, size_t offset) { return -ENOSYS; (void)buf; (void)len; (void)offset; }
    virtual int write(void* buf, size_t len, size_t offset) { return -ENOSYS; (void)buf; (void)len; (void)offset; }

    int block_device_id;

    uint64_t len;
};

struct PartitionBlockDevice : public BlockDevice {
    int read(void* buf, size_t len, size_t _offset) override;
    int write(void* buf, size_t len, size_t _offset) override;

    BlockDevice* main;
    size_t offset;
    uint64_t guid[2];
};

// TODO: implement a hda1,2,3,4,.... style thing

class BlockManager {
public:
    int RegisterBlockDevice(BlockDevice* device);
    BlockDevice* GetBlockDevice(int block_id, int part_id);
    void ParsePartitions();

    static BlockManager& the() {
        static BlockManager instance;
        return instance;
    }

private:
    struct BlockDeviceContainer {
        BlockDevice* main;
        Vector<BlockDevice*> partitions;
        bool parsed = false;
    };

    Vector<BlockDeviceContainer*> devices;
    int next_id = 0;
};

}

#endif
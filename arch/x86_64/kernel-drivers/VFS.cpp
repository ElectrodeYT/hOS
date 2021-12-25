#include <kernel-drivers/VFS.h>
#include <mem/VM/virtmem.h>
#include <debug/klog.h>
#include <errno.h>

namespace Kernel {

int EchFSDriver::read(VFS::fs_node* node, void* buf, size_t size, size_t offset) {
    if(!mounted) { return -EINVAL; }
    (void)node; (void)buf; (void)size; (void)offset;
    return -ENOSYS;
}
int EchFSDriver::write(VFS::fs_node* node, void* buf, size_t size, size_t offset) {
    if(!mounted) { return -EINVAL; }
    (void)node; (void)buf; (void)size; (void)offset;
    return -ENOSYS;
}
int EchFSDriver::open(VFS::fs_node* node, bool _read, bool _write) {
    if(!mounted) { return -EINVAL; }
    (void)node; (void)_read; (void)_write;
    return -ENOSYS;
}
int EchFSDriver::close(VFS::fs_node* node) {
    if(!mounted) { return -EINVAL; }
    (void)node; 
    return -ENOSYS;
}
VFS::dirent* EchFSDriver::readdir(VFS::fs_node* node, size_t num) {
    if(!mounted) { return NULL; }
    (void)node; (void)num;
    return NULL;
}
VFS::fs_node* EchFSDriver::finddir(VFS::fs_node* node, const char* name) {
    if(!mounted) { return NULL; }
    (void)node; (void)name;
    return NULL;
}

bool EchFSDriver::mount() {
    if(!block) { return false; }
    // Read the first bytes and check if its actually EchFS
    uint8_t* identity_table = new uint8_t[56];
    uint8_t sig[8] = { '_', 'E', 'C', 'H', '_', 'F', 'S', '_' };
    block->read(identity_table, 56, 0);
    // Compare the signature
    if(*((uint64_t*)sig) != *((uint64_t*)(identity_table + 4))) { return false; }
    // We have a EchFS partition here
    block_count = *((uint64_t*)(identity_table + 8));
    block_size = *((uint64_t*)(identity_table + 24));
    allocation_table_block_size = (block_count * sizeof(uint64_t) + block_size - 1) / block_size;
    // Some logic here if the table is actually fucking giant to allocate it with pages
    // instead of the kernel heap
    if((allocation_table_block_size * block_size) > (16 * 1024)) {
        allocation_table = (uint64_t*)VM::Manager::the().AllocatePages((allocation_table_block_size * block_size) / 4096);
        is_allocation_table_on_heap = false;
    } else {
        allocation_table = new uint64_t[allocation_table_block_size * (block_size / sizeof(uint64_t))];
        is_allocation_table_on_heap = true;
    }
    if(!allocation_table) {
        KLog::the().printf("EchFSDriver: failed to allocate allocation table\n\r");
    }
    // Read the allocation table
    block->read(allocation_table, (allocation_table_block_size * block_size), 16 * block_size);
    KLog::the().printf("EchFSDriver: detected echfs, block size %i, block count %i\n\r", block_size, block_count);
    return true;
}

}
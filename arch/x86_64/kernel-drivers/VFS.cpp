#include <kernel-drivers/VFS.h>
#include <mem/VM/virtmem.h>
#include <debug/klog.h>
#include <CPP/string.h>
#include <errno.h>
#include <mem.h>

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
    // Max file name len on echfs is 200
    if(strlen(name) >= 200) { return NULL; }
    // node must be a dir or a mount
    if(node->flags != FS_NODE_DIR && node->flags != (FS_NODE_DIR | FS_NODE_MOUNT)) { return NULL; }
    uint64_t entry_count = main_dir_entry_len();
    for(size_t i = 0; i < entry_count; i++) {
        echfs_dir_entry* entry = &main_directory_table[i];
        if(entry->dir_id != node->inode) { continue; }
        if(strcmp(entry->name, name) == 0) {
            // Check if we have this file cached
            echfs_file_ll* curr = cached_file_entries;
            while(curr) {
                echfs_file* cache = curr->file;
                if(cache->dir_entry == i) {
                    return cache->node;
                }
            }
            // We do not have a cached entry for this, create a echfs_file and a fs_node for this
            echfs_file* file_entry = new echfs_file;
            VFS::fs_node* new_node = new VFS::fs_node;
            new_node->driver = this;
            new_node->flags = entry->type ? FS_NODE_DIR : FS_NODE_FILE;
            new_node->inode = entry->type ? entry->starting_block : i;
            new_node->length = entry->type ? 0 : entry->file_size;
            new_node->uid = entry->owner;
            new_node->gid = entry->group;
            new_node->mask = entry->permissions;
            new_node->open_count = 0;
            
            file_entry->node = new_node;
            file_entry->dir_entry = i;
            file_entry->dir_id = entry->type ? entry->starting_block : 0;
            file_entry->opened = false;
            
            echfs_file_ll* ll_entry = new echfs_file_ll;
            ll_entry->file = file_entry;
            ll_entry->next = cached_file_entries;
            cached_file_entries = ll_entry;

            return new_node;
        }
    }
    return NULL;
}

VFS::fs_node* EchFSDriver::mount() {
    if(!block) { return NULL; }
    if(mounted) {
        if(root_node) { return root_node; }
        return NULL;
    }
    // Read the first bytes and check if its actually EchFS
    uint8_t* identity_table = new uint8_t[56];
    uint8_t sig[8] = { '_', 'E', 'C', 'H', '_', 'F', 'S', '_' };
    block->read(identity_table, 56, 0);
    // Compare the signature
    if(*((uint64_t*)sig) != *((uint64_t*)(identity_table + 4))) { return NULL; }
    // We have a EchFS partition here
    block_count = *((uint64_t*)(identity_table + 12));
    block_size = *((uint64_t*)(identity_table + 28));
    allocation_table_block_size = (block_count * sizeof(uint64_t) + block_size - 1) / block_size;
    main_directory_block_size = *((uint64_t*)(identity_table + 20));
    KLog::the().printf("EchFSDriver: detected echfs, block size %i, block count %i\n\r", block_size, block_count);
    KLog::the().printf("EchFSDriver: allocation table block count %i\n\r", allocation_table_block_size);
    // Some logic here if the table is actually fucking giant to allocate it with pages
    // instead of the kernel heap
    if((allocation_table_block_size * block_size) > (16 * 1024)) {
        KLog::the().printf("EchFSDriver: attempting to allocate %x bytes for the allocation table buffer\n\r", (allocation_table_block_size * block_size));
        allocation_table = (uint64_t*)VM::Manager::the().AllocatePages((allocation_table_block_size * block_size) / 4096);
        is_allocation_table_on_heap = false;
    } else {
        KLog::the().printf("EchFSDriver: attempting to allocate %x bytes on the heap for the allocation table buffer\n\r", (allocation_table_block_size * block_size));
        allocation_table = new uint64_t[allocation_table_block_size * (block_size / sizeof(uint64_t))];
        is_allocation_table_on_heap = true;
    }
    if((main_directory_block_size * block_size) > (16 * 1024)) {
        KLog::the().printf("EchFSDriver: attempting to allocate %x bytes for the main dir table buffer\n\r", (main_directory_block_size * block_size));
        main_directory_table = (echfs_dir_entry*)VM::Manager::the().AllocatePages((main_directory_block_size * block_size) / 4096);
        is_main_directory_table_on_heap = false;
    } else {
        KLog::the().printf("EchFSDriver: attempting to allocate %x bytes on the heap for the main dir table buffer\n\r", (main_directory_block_size * block_size));
        main_directory_table = new echfs_dir_entry[(main_directory_block_size * block_size) / sizeof(echfs_dir_entry)];
        is_main_directory_table_on_heap = true;
    }

    if(!main_directory_table) {
        KLog::the().printf("EchFSDriver: failed to allocate allocation table\n\r");
    }
    // Read the allocation table
    block->read(allocation_table, (allocation_table_block_size * block_size), 16 * block_size);
    // Read the main dir table
    block->read(main_directory_table, (main_directory_block_size * block_size), (16 + allocation_table_block_size) * block_size);
    // Dump the allocation table stats to KLog
    dumpBlockStates();
    // Create and read root node
    root_node = new VFS::fs_node;
    root_node->name = NULL;
    root_node->driver = this;
    root_node->inode = 0xFFFFFFFFFFFFFFFF;
    root_node->flags = FS_NODE_DIR | FS_NODE_MOUNT;
    dumpMainDirectory();
    mounted = true;
    return root_node;
}

void EchFSDriver::dumpBlockStates() {
    uint64_t used_blocks = 0;
    uint64_t free_blocks = 0;
    uint64_t reserved_blocks = 0;
    for(size_t blk = 0; blk < block_count; blk++) {
        if(allocation_table[blk]) {
            if(allocation_table[blk] == 0xFFFFFFFFFFFFFFF0) { reserved_blocks++; } else { used_blocks++; }
        } else {
            free_blocks++;
        }
    }
    KLog::the().printf("EchFSDriver: %i used blocks, %i reserved blocks, %i free blocks\n\r", used_blocks, reserved_blocks, free_blocks);
}

void EchFSDriver::dumpMainDirectory() {
    KLog::the().printf("EchFSDriver: root folder contents: \n\r");
    for(size_t i = 0; i < ((main_directory_block_size * block_size) / 256); i++) {
        echfs_dir_entry entry = main_directory_table[i];
        if(entry.dir_id == 0xFFFFFFFFFFFFFFFF) {
            if(entry.type) {
                KLog::the().printf("EchFSDriver: sub dir %s, id %x\n\r", entry.name, entry.starting_block);
            } else {
                KLog::the().printf("EchFSDriver: file %s, starting block %x, size %x\n\r", entry.name, entry.starting_block, entry.file_size);
            }
        }
    }
}

}
#include <kernel-drivers/VFS.h>
#include <mem/VM/virtmem.h>
#include <debug/klog.h>
#include <CPP/string.h>
#include <errno.h>
#include <mem.h>

namespace Kernel {

bool VFS::attemptMountRoot(VFSDriver* driver) {
    if(root_node) { return false; }
    root_node = driver->mount();
    if(!root_node) {
        KLog::the().printf("VFS: couldnt mount root fs\n\r");
        return false;
    }
    return true;
}

VFS::fs_node* getNodeFromPath(const char* str) {
    return NULL; // TODO: getNodeFromPath
    (void)str;
}

int VFS::fs_node::read(void* buf, size_t size, size_t offset) {
    if(!driver) { return -EINVAL; }
    return driver->read(this, buf, size, offset);
}
int VFS::fs_node::write(void* buf, size_t size, size_t offset) {
    if(!driver) { return -EINVAL; }
    return driver->write(this, buf, size, offset);
}
int VFS::fs_node::open(bool _read, bool _write) {
    if(!driver) { return -EINVAL; }
    return driver->open(this, _read, _write);
}
int VFS::fs_node::close() {
    if(!driver) { return -EINVAL; }
    return driver->close(this);
}
VFS::dirent* VFS::fs_node::readdir(size_t num) {
    if(!driver) { return NULL; }
    return driver->readdir(this, num);
}
VFS::fs_node* VFS::fs_node::finddir(const char* name) {
    if(!driver) { return NULL; }
    return driver->finddir(this, name);
}

///
/// EchFS stuff
///
int EchFSDriver::read(VFS::fs_node* node, void* buf, size_t size, size_t offset) {
    if(!mounted) { return -EINVAL; }
    // We cant read a directory lol
    if(node->flags == FS_NODE_DIR || node->flags == (FS_NODE_DIR & FS_NODE_MOUNT)) { return -EINVAL; }
    // Find the echfs_file entry we have for this
    echfs_file* file = NULL;
    echfs_file_ll* curr_ll = cached_file_entries;
    while(curr_ll) {
        if(curr_ll->file->node == node) { file = curr_ll->file; break; }
        curr_ll = curr_ll->next;
    }
    if(!file) { return -EINVAL; }
    // Constrain size
    if((offset + size) > main_directory_table[file->dir_entry].file_size) {
        uint64_t new_size = main_directory_table[file->dir_entry].file_size - offset;
        KLog::the().printf("EchFSDriver: read of size %i constrained to %i\n\r", size, new_size);
        size = new_size;
    }

    uint64_t end = offset + size;
    // Check if we have the blocks we need buffered
    if(file->known_blocks.size() < (end % block_size) + 1) {
        // Special case if we have no blocks buffered
        if(file->known_blocks.size() == 0) {
            uint64_t file_dir_entry = file->dir_entry;
            file->known_blocks.push_back(main_directory_table[file_dir_entry].starting_block);
        }
        // Traverse the blocks we need to get to the blocks we need
        for(size_t curr_block = file->known_blocks.size(); curr_block < ((end / block_size) + 1); curr_block++) {
            // Get the previous block
            uint64_t prev_block_id = file->known_blocks.at(curr_block - 1);
            // Read the next block
            uint64_t curr_block_id = allocation_table[prev_block_id];
            if(curr_block_id == 0) {
                KLog::the().printf("EchFSDriver: file seems to have blocks pointing to free area\n\r");
                return -ENOENT;
            } else if(curr_block_id == 0xFFFFFFFFFFFFFFF0) {
                KLog::the().printf("EchFSDriver: file seemt to have blocks pointing to reserved area\n\r");
                return -ENOENT;
            } else if(curr_block_id == 0xFFFFFFFFFFFFFFFF) {
                KLog::the().printf("EchFSDriver: file read points to blocks after the end of the chain\n\r");
                // Shorten size
                size = ((curr_block - 1) * block_size) - offset;
                break;
            }
            file->known_blocks.push_back(curr_block_id);
        }
    }
    // We load the buffer each logical block at a time
    uint64_t curr = offset;
    uint64_t first_block_offset = (offset % block_size);
    // The amount of data we still have to read
    uint64_t remaining_len = size;
    uint8_t* curr_buf = (uint8_t*)buf;
    while(end > curr) {
        // Calculate needed block
        uint64_t block_id = file->known_blocks.at(curr / block_size);
        // Calculate the amount of data we need from this block
        uint64_t block_len = ((block_size - first_block_offset) <= remaining_len) ? (block_size - first_block_offset) : remaining_len;
        // Read this data into the buffer
        block->read(curr_buf, block_len, block_id * block_size);
        remaining_len -= block_len;
        curr += block_len;
        first_block_offset = 0;
    }
    return size;
}
int EchFSDriver::write(VFS::fs_node* node, void* buf, size_t size, size_t offset) {
    if(!mounted) { return -EINVAL; }
    (void)node; (void)buf; (void)size; (void)offset;
    return -ENOSYS;
}
// TODO: does the driver even need to know of the opens and closes?
// It might if it would flush buffers and things, but we dont do that yet
int EchFSDriver::open(VFS::fs_node* node, bool _read, bool _write) {
    if(!mounted || !node) { return -EINVAL; }
    node->open_count++;
    return 0;
    (void)_read; (void)_write;
}
int EchFSDriver::close(VFS::fs_node* node) {
    if(!mounted || !node) { return -EINVAL; }
    if(node->open_count > 0) { node->open_count--; }
    else { KLog::the().printf("EchFSDriver: close called with no open FDs"); }
    // If no one has this file open, we can discard the known blocks vector
    if(node->open_count == 0) {
        // Find the echfs_file entry we have for this
        echfs_file* file = NULL;
        echfs_file_ll* curr_ll = cached_file_entries;
        while(curr_ll) {
            if(curr_ll->file->node == node) { file = curr_ll->file; break; }
            curr_ll = curr_ll->next;
        }
        if(!file) { return 0; } // If we cant find it, then just return
        file->known_blocks.clear_and_free();
    }
    return 0;
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
        allocation_table = (uint64_t*)VM::AllocatePages(((allocation_table_block_size * block_size) / 4096) + (((allocation_table_block_size * block_size) % 4096) ? 1 : 0));
        is_allocation_table_on_heap = false;
    } else {
        KLog::the().printf("EchFSDriver: attempting to allocate %x bytes on the heap for the allocation table buffer\n\r", (allocation_table_block_size * block_size));
        allocation_table = new uint64_t[allocation_table_block_size * (block_size / sizeof(uint64_t))];
        is_allocation_table_on_heap = true;
    }
    if((main_directory_block_size * block_size) > (16 * 1024)) {
        KLog::the().printf("EchFSDriver: attempting to allocate %x bytes for the main dir table buffer\n\r", (main_directory_block_size * block_size));
        main_directory_table = (echfs_dir_entry*)VM::AllocatePages(((main_directory_block_size * block_size) / 4096) + (((main_directory_block_size * block_size) % 4096) ? 1 : 0));
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
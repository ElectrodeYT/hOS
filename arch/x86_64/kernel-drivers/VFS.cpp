#include <kernel-drivers/VFS.h>
#include <mem/VM/virtmem.h>
#include <debug/klog.h>
#include <CPP/string.h>
#include <errno.h>
#include <mem.h>

namespace Kernel {

size_t getLenUntilEndOfPathPart(const char* s) {
    char* curr = (char*)s;
    size_t len = 0;
    while(true) {
        if(*curr == '/' || *curr == 0) { break; }
        len++;
        curr++;
    }
    return len;
}

bool VFS::attemptMountRoot(VFSDriver* driver) {
    if(root_node) { return false; }
    root_node = driver->mount();
    if(!root_node) {
        return false;
    }
    KLog::the().printf("VFS: mounted %s as root\n\r", driver->driverName());
    return true;
}

VFS::fs_node* VFS::iterateFromDescriptor(const char* working, fs_node* node, int64_t* err) {
    fs_node* curr = node;
    // We need to iterate over the working path first
    if(working[0] != '/') { *err = -EINVAL; return NULL; }
    char* curr_folder_name = NULL;
    char* curr_working_index = (char*)(working + 1);
    while(*curr_working_index) {
        // Get the size of this file name
        size_t char_len = getLenUntilEndOfPathPart(curr_working_index);
        if(char_len > 256) { *err = -ENAMETOOLONG; return NULL;  }
        curr_folder_name = new char[char_len + 1];
        memset(curr_folder_name, 0, char_len + 1);
        memcopy(curr_working_index, curr_folder_name, char_len);
        fs_node* next = curr->finddir(curr_folder_name);
        delete curr_folder_name;
        if(!next) { *err = -EINVAL; }
        if(!next->isDir()) { *err = -ENOTDIR; return NULL;  }
        curr = next;
        curr_working_index += char_len;
    }
    return curr;
}

int VFS::attemptMountOnFolder(const char* working, const char* folder, VFSDriver* driver) {
    acquire(&mutex);
    fs_node* curr = root_node;
    // Check if we have a relative path
    if(folder[0] != '/') {
        int64_t err = 0;
        iterateFromDescriptor(working, curr, &err);
        if(err) { release(&mutex); return err; }
    }
    // Iterate over the file relative path
    char* curr_folder_name = NULL;
    char* curr_file_index = (char*)folder;
    if(*curr_file_index == '/') { curr_file_index++; }
    while(*curr_file_index) {
        if(!curr->isDir()) { release(&mutex); return -ENOTDIR; }
        // Get the size of this file name
        size_t char_len = getLenUntilEndOfPathPart(curr_file_index);
        if(char_len > 256) { release(&mutex); return -ENAMETOOLONG; }
        curr_folder_name = new char[char_len + 1];
        memset(curr_folder_name, 0, char_len + 1);
        memcopy(curr_file_index, curr_folder_name, char_len);
        fs_node* next = root_node->finddir(curr_folder_name);
        delete curr_folder_name;
        if(!next) { release(&mutex); return -EINVAL; }
        curr = next;
        curr_file_index += char_len;
    }
    if(!curr->isDir()) { release(&mutex); return false; }
    fs_node* mount = driver->mount();
    if(mount) {
        // We can mount this driver, overwrite the curr node
        memcopy(mount, curr, sizeof(fs_node));
        if(*folder == '/') { folder++; }
        if(working[1]) {
            KLog::the().printf("VFS: mounted %s on %s/%s\n\r", driver->driverName(), working, folder);
        } else {
            KLog::the().printf("VFS: mounted %s on /%s\n\r", driver->driverName(), folder);    
        }
        release(&mutex);
        return true;
    }
    release(&mutex);
    return false;
}

int64_t VFS::open(const char* working, const char* file, int64_t pid) {
    acquire(&mutex);
    fs_node* curr = root_node;
    // Check if we have a relative path
    if(file[0] != '/') {
        // We need to iterate over the working path first
        if(working[0] != '/') { release(&mutex); return -EINVAL; }
        char* curr_folder_name = NULL;
        char* curr_working_index = (char*)(working + 1);
        while(*curr_working_index) {
            // Get the size of this file name
            size_t char_len = getLenUntilEndOfPathPart(curr_working_index);
            if(char_len > 256) { release(&mutex); return -ENAMETOOLONG; }
            curr_folder_name = new char[char_len + 1];
            memset(curr_folder_name, 0, char_len + 1);
            memcopy(curr_working_index, curr_folder_name, char_len);
            fs_node* next = curr->finddir(curr_folder_name);
            delete curr_folder_name;
            if(!next) { release(&mutex); return -EINVAL; }
            if(!next->isDir()) { release(&mutex); return -ENOTDIR; }
            curr = next;
            curr_working_index += char_len;
            if(*curr_working_index == '/') {
                if(!curr->isDir()) { release(&mutex); return -ENOTDIR; }
                curr_working_index++;
            }
        }
    }
    // Iterate over the file relative path
    char* curr_folder_name = NULL;
    char* curr_file_index = (char*)file;
    if(*curr_file_index == '/') { curr_file_index++; }
    if(*curr_file_index == 0) { release(&mutex); return -EISDIR; }
    while(*curr_file_index) {
        if(!curr->isDir()) { release(&mutex); return -ENOTDIR; }
        // Get the size of this file name
        size_t char_len = getLenUntilEndOfPathPart(curr_file_index);
        if(char_len > 256) { release(&mutex); return -ENAMETOOLONG; }
        curr_folder_name = new char[char_len + 1];
        memset(curr_folder_name, 0, char_len + 1);
        memcopy(curr_file_index, curr_folder_name, char_len);
        fs_node* next = curr->finddir(curr_folder_name);
        delete curr_folder_name;
        if(!next) { release(&mutex); return -EINVAL; }
        curr = next;
        curr_file_index += char_len;
        if(*curr_file_index == '/') {
            if(!curr->isDir()) { release(&mutex); return -ENOTDIR; }
            curr_file_index++;
        }
    }
    if(curr->isDir()) { release(&mutex); return -EISDIR; }
    // Create a new file descriptor for this file
    int64_t file_desc = 0;
    for(int64_t i = 0; i < (int64_t)opened_files.size(); i++) {
        if(opened_files.at(i)->id == file_desc) { file_desc++; i = -1; }
    }
    // file_desc is now the lowest unused file descriptor, time to create a new fileDescriptor
    fileDescriptor* fd = new fileDescriptor;
    fd->id = file_desc;
    fd->pid = pid;
    fd->node = curr;
    opened_files.push_back(fd);
    release(&mutex);
    return file_desc;
}

int VFS::close(int64_t file, int64_t pid) {
    acquire(&mutex);
    // Find the fd
    for(size_t i = 0; i < opened_files.size(); i++) {
        if(opened_files.at(i)->id == file) {
            // If we are not the kernel (pid -1) then we also check the pid
            if(pid != -1 && opened_files.at(i)->pid != pid) {
            //    return -EBADF;
            }
            delete opened_files.at(i);
            opened_files.remove(i);
            release(&mutex);
            return 0;
        }
    }
    release(&mutex);
    return -EBADF;
}

int VFS::pread(int64_t file, void* buf, size_t nbyte, size_t offset, int64_t pid) {
    // Find the fd
    fs_node* node = NULL;
    for(size_t i = 0; i < opened_files.size(); i++) {
        if(opened_files.at(i)->id == file) {
            // If we are not the kernel (pid -1) then we also check the pid
            if(pid != -1 && opened_files.at(i)->pid != pid) {
            //    return -EBADF;
            }
            node = opened_files.at(i)->node;
            break;
        }
    }
    if(!node) { return -EBADF; }
    int ret = node->read(buf, nbyte, offset);
    return ret;
}

int VFS::pwrite(int64_t file, void* buf, size_t nbyte, size_t offset, int64_t pid) {
    // Find the fd
    fs_node* node = NULL;
    for(size_t i = 0; i < opened_files.size(); i++) {
        if(opened_files.at(i)->id == file) {
            // If we are not the kernel (pid -1) then we also check the pid
            if(pid != -1 && opened_files.at(i)->pid != pid) {
            //    return -EBADF;
            }
            node = opened_files.at(i)->node;
            break;
        }
    }
    if(!node) { return -EBADF; }
    int ret = node->write(buf, nbyte, offset);
    return ret;
}

size_t VFS::size(int64_t file, int64_t pid) {
    // Find the fd
    fs_node* node = NULL;
    for(size_t i = 0; i < opened_files.size(); i++) {
        if(opened_files.at(i)->id == file) {
            // If we are not the kernel (pid -1) then we also check the pid
            if(pid != -1 && opened_files.at(i)->pid != pid) {
                return -EBADF;
            }
            node = opened_files.at(i)->node;
            break;
        }
    }
    if(!node) { return -EBADF; }
    return node->size();
}

int VFS::fs_node::read(void* buf, size_t size, size_t offset) {
    // Check if a node is a character device
    if(flags == FS_NODE_CHARDEVICE) {
        // It is, read from it.
        // TODO: what to do with offset?
        CharDevice* char_dev = CharDeviceManager::the().get(block_char_dev_id);
        if(!char_dev) { return -EINVAL; }
        return char_dev->read((char*)buf, size);
    } else if(flags == FS_NODE_BLOCKDEVICE) {
        // If its a block device, just read from the block device
        BlockDevice* block_dev = BlockManager::the().GetBlockDevice(block_char_dev_id, block_dev_part_id);
        if(!block_dev) { return -EINVAL; }
        return block_dev->read(buf, size, offset);
    }
    if(!driver) { return -EINVAL; }
    return driver->read(this, buf, size, offset);
}
int VFS::fs_node::write(void* buf, size_t size, size_t offset) {
    if(flags == FS_NODE_CHARDEVICE) {
        CharDevice* char_dev = CharDeviceManager::the().get(block_char_dev_id);
        if(!char_dev) { return -ENOTTY; }
        return char_dev->write((char*)buf, size);
    } else if(flags == FS_NODE_BLOCKDEVICE) {
        BlockDevice* block_dev = BlockManager::the().GetBlockDevice(block_char_dev_id, block_dev_part_id);
        if(!block_dev) { return -ENOTBLK; }
        return block_dev->write(buf, size, offset);
    }
    if(!driver) { return -EINVAL; }
    return driver->write(this, buf, size, offset);
}
int VFS::fs_node::open(bool _read, bool _write) {
    if(flags == FS_NODE_CHARDEVICE || flags == FS_NODE_BLOCKDEVICE) {
        open_count++;
        return 0;
    }
    if(!driver) { return -EINVAL; }
    return driver->open(this, _read, _write);
}
int VFS::fs_node::close() {
    if(flags == FS_NODE_CHARDEVICE || flags == FS_NODE_BLOCKDEVICE) {
        open_count--;
        return 0;
    }
    if(!driver) { return -EINVAL; }
    return driver->close(this);
}
size_t VFS::fs_node::size() {
    if(flags == FS_NODE_CHARDEVICE || flags == FS_NODE_BLOCKDEVICE) {
        return -ENOSYS;
    }
    if(!driver) { return -EINVAL; }
    return driver->size(this);
}
VFS::dirent* VFS::fs_node::readdir(size_t num) {
    if(flags == FS_NODE_CHARDEVICE || flags == FS_NODE_BLOCKDEVICE) {
        return NULL;
    }
    if(!driver) { return NULL; }
    return driver->readdir(this, num);
}
VFS::fs_node* VFS::fs_node::finddir(const char* name) {
    if(flags == FS_NODE_CHARDEVICE || flags == FS_NODE_BLOCKDEVICE) {
        return NULL;
    }
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
    if(file->known_blocks.size() < (end / block_size) + 1) {
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
            // Debug loop for if curr_block_id == 0
            if(curr_block_id == 0) { 
                KLog::the().printf("debug check: curr_block_id == 0"); for(;;);
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
        block->read(curr_buf, block_len, (block_id * block_size) + first_block_offset);
        remaining_len -= block_len;
        curr += block_len;
        curr_buf += block_len;
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
size_t EchFSDriver::size(VFS::fs_node* node) {
    if(!mounted || !node) { return -EINVAL; }
    if(node->isDir()) { return -EISDIR; }
    return main_directory_table[node->inode].file_size;
}
VFS::dirent* EchFSDriver::readdir(VFS::fs_node* node, size_t num) {
    if(!mounted) { return NULL; }
    if(node->flags != FS_NODE_DIR && node->flags != (FS_NODE_DIR | FS_NODE_MOUNT)) { return NULL; }
    uint64_t entry_count = main_dir_entry_len();
    uint64_t curr_num = 0;
    for(size_t i = 0; i < entry_count; i++) {
        echfs_dir_entry* entry = &main_directory_table[i];
        if(entry->dir_id != node->inode) { continue; }
        if(curr_num < num) { curr_num++; continue; }
        // We have gotten the entry num we want
        VFS::dirent* ret = new VFS::dirent;
        memcopy(entry->name, ret->name, 201);
        ret->inode = i;
    }
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
                curr = curr->next;
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

///
/// DevFS stuff
///
// The read, write, open and close functions will only ever be called on the rootfs
// the VFS::fs_node functions deal with reading/writing to block/char devices
int DevFSDriver::read(VFS::fs_node* node, void* buf, size_t size, size_t offset) {
    return -EISDIR;
    (void)node; (void)buf; (void)size; (void)offset;
}

int DevFSDriver::write(VFS::fs_node* node, void* buf, size_t size, size_t offset) {
    return -EISDIR;
    (void)node; (void)buf; (void)size; (void)offset;
}

int DevFSDriver::open(VFS::fs_node* node, bool read, bool write) {
    return -EISDIR;
    (void)node; (void)read; (void)write;
}

int DevFSDriver::close(VFS::fs_node* node) {
    return -EISDIR;
    (void)node;
}

VFS::dirent* DevFSDriver::readdir(VFS::fs_node* node, size_t num) {
    acquire(&mutex);
    (void)node; (void)num;
    release(&mutex);
    return NULL;
}

VFS::fs_node* DevFSDriver::finddir(VFS::fs_node* node, const char* name) {
    acquire(&mutex);
    // we dont have subdirs
    if(node->inode != 0xFFFFFFFFFFFFFFFF || !(node->isDir())) { return NULL; }
    // in any case, name must be at least 3 chars
    size_t name_len = strlen(name);
    if(name_len < 3) { return NULL; }
    // check the name
    if(name[0] == 't' && name[1] == 't' && name[2] == 'y' ) {
        // we still need a number, new length minimum is 4
        if(name_len < 4) { return NULL; }
        const char* number_part = (name + 3);
        long tty_id = atoi(number_part);
        if(tty_id <= 0) { return NULL; }
        tty_id--;
        // Check if a tty for this even exists
        if(!CharDeviceManager::the().get(tty_id)) { return NULL; }
        // Attempt to get the tty for this
        for(size_t i = 0; i < tty_nodes.size(); i++) {
            if(tty_nodes.at(i)->tty_id == tty_id) { 
                VFS::fs_node* node = tty_nodes.at(i)->node;
                release(&mutex);
                return node;
            }
        }
        // We dont have a node for this, make one
        ttyContainer* container = new ttyContainer;
        container->tty_id = tty_id;
        container->node = new VFS::fs_node;
        container->node->inode = tty_id | tty_bitmask;
        container->node->flags = FS_NODE_CHARDEVICE;
        container->node->driver = this;
        container->node->block_char_dev_id = tty_id;
        container->node->block_dev_part_id = 0;
        tty_nodes.push_back(container);
        release(&mutex);
        return container->node;
    }
    release(&mutex);
    return NULL;
}

VFS::fs_node* DevFSDriver::mount() {
    // We just create a special directory inode for this lol
    root_node = new VFS::fs_node;
    root_node->driver = this;
    root_node->flags = FS_NODE_DIR | FS_NODE_MOUNT;
    root_node->inode = 0xFFFFFFFFFFFFFFFF;
    root_node->uid = 0;
    root_node->gid = 0;
    return root_node;
}

}
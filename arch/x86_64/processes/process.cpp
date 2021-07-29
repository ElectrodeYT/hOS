#include <mem.h>
#include <processes/process.h>
#include <mem/VM/virtmem.h>
#include <debug/serial.h>

namespace Kernel  {

namespace Processes {

bool Process::attemptCopyFromUser(uint64_t user_pointer, size_t size, void* destination) {
    // Perform bounds check for beginning and end
    bool pointer_valid = false;
    // Debug::SerialPrintf("Attempting to copy from %s memory\r\n", name);
    // Debug::SerialPrintf("Mapping count: %i\r\n", mappings.size());
    for(size_t i = 0; i < mappings.size(); i++) {
        // Check if beginning fits in this mapping
        if(mappings.at(i)->base <= user_pointer) {
            // Check if end fits
            if((mappings.at(i)->base + mappings.at(i)->size) >= (user_pointer + size)) {
                pointer_valid = true;
                break;
            }
        }
    }
    // Check if we actually have a valid mapping
    if(!pointer_valid) { return false; }
    // Switch to process memory
    uint64_t current_page_table = VM::Manager::the().CurrentPageTable();
    VM::Manager::the().SwitchPageTables(page_table);
    memcopy((void*)user_pointer, destination, size);
    VM::Manager::the().SwitchPageTables(current_page_table);
    return true;
}

}

}
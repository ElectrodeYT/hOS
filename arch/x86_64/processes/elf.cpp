#include <mem.h>
#include <processes/elf.h>
#include <debug/klog.h>

// TODO: perform size checks to make sure all sizes are in bounds
bool ELF::readHeader() {
    size_t curr_pointer = 0;
    // Read the first 4 bytes
    char mag0 = (char)read8(curr_pointer++);
    char mag1 = (char)read8(curr_pointer++);
    char mag2 = (char)read8(curr_pointer++);
    char mag3 = (char)read8(curr_pointer++);
    // Check if the magic is correct
    if(mag0 != '\x7f' || mag1 != 'E' || mag2 != 'L' || mag3 != 'F') { return false; }

    // We only load ELF64 files
    if(read8(4) != 2) { return false; } // e_ident[EI_CLASS] should be 2 for ELF64
    // Get endianness
    if(read8(5) == 2) { endianness = true; } else { endianness = false; }


    file_version = read8(6);
    file_abi = read8(7);
    file_abi_version = read8(8);
    file_padding_byte_start = read8(9);

    // Kernel::KLog::the().printf("Read ELF header, %s endian, %s ABI\r\n", endianness ? "Big" : "Little", getABI());

    if(endianness) { Kernel::KLog::the().printf("TODO: support big endian elf files\r\n"); return false; }

    curr_pointer = 16;
    file_object_type = (ObjectFileType)read16(curr_pointer); curr_pointer += 2;
    file_machine_type = read16(curr_pointer); curr_pointer += 2;
    file_object_version = read32(curr_pointer); curr_pointer += 4;
    file_entry = read64(curr_pointer); curr_pointer += 8;
    file_program_header_offset = read64(curr_pointer); curr_pointer += 8;
    file_section_header_offset = read64(curr_pointer); curr_pointer += 8;
    file_processor_flags = read32(curr_pointer); curr_pointer += 4;
    file_header_size = read16(curr_pointer); curr_pointer += 2;
    file_program_header_entry_size = read16(curr_pointer); curr_pointer += 2;
    file_program_header_count = read16(curr_pointer); curr_pointer += 2;
    file_section_header_entry_size = read16(curr_pointer); curr_pointer += 2;
    file_section_header_count = read16(curr_pointer); curr_pointer += 2;
    file_section_name_string_table_index = read16(curr_pointer); curr_pointer += 2;

    /*Kernel::Debug::SerialPrint("Object file type: ");
    switch(file_object_type) {
        case ET_NONE: Kernel::Debug::SerialPrint("ET_NONE\r\n"); break;
        case ET_REL: Kernel::Debug::SerialPrint("ET_REL\r\n"); break;
        case ET_EXEC: Kernel::Debug::SerialPrint("ET_EXEC\r\n"); break;
        case ET_DYN: Kernel::Debug::SerialPrint("ET_DYN\r\n"); break;
        case ET_CORE: Kernel::Debug::SerialPrint("ET_CORE\r\n"); break;
    }
*/
    // Kernel::KLog::the().printf("Header size: %x\r\nEntry point: %x\r\nProgram header offset: %x\r\nSection header offset: %x\r\n", file_header_size, file_entry, file_program_header_offset, file_section_header_offset);

    // We now need to read the program header tables
    curr_pointer = file_program_header_offset;
    for(size_t i = 0; i < file_program_header_count; i++) {
        uint32_t type = read32(curr_pointer); curr_pointer += 4;
        uint32_t flags = read32(curr_pointer); curr_pointer += 4;
        uint64_t offset_in_file = read64(curr_pointer); curr_pointer += 8;
        uint64_t vaddr = read64(curr_pointer); curr_pointer += 8;
        curr_pointer += 8; // paddr (reserved)
        uint64_t size_in_file = read64(curr_pointer); curr_pointer += 8;
        uint64_t size_in_memory = read64(curr_pointer); curr_pointer += 8;
        curr_pointer += 8; // we trust the alignment is write 
        // TODO: dont trust the aligment

        Section* new_section = new Section(data);
        new_section->file_begin = offset_in_file;
        new_section->size = size_in_file;
        new_section->segment_size = size_in_memory;
        new_section->vaddr = vaddr;
        new_section->read = flags & 0b100;
        new_section->write = flags & 0b10;
        new_section->execute = flags & 0b1;
        new_section->loadable = type == 1;
        sections.push_back(new_section);
        Kernel::KLog::the().printf("Segment %i: vaddr %x, file size %x, segment size %x, type %i\r\n", i, vaddr, size_in_file, size_in_memory, type);
        Kernel::KLog::the().printf("Offset in file: %x\r\n", offset_in_file);
    }


    return true;
}
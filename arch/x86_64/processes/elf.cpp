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

    //Kernel::KLog::the().printf("Read ELF header, %s endian, %s ABI\r\n", endianness ? "Big" : "Little", getABI());

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

    //Kernel::KLog::the().printf("Object file type: ");
    switch(file_object_type) {
        case ET_NONE: Kernel::KLog::the().printf("ET_NONE\r\n"); break;
        case ET_REL: Kernel::KLog::the().printf("ET_REL\r\n"); break;
        case ET_EXEC: Kernel::KLog::the().printf("ET_EXEC\r\n"); break;
        case ET_DYN: Kernel::KLog::the().printf("ET_DYN\r\n"); break;
        case ET_CORE: Kernel::KLog::the().printf("ET_CORE\r\n"); break;
    }

    //Kernel::KLog::the().printf("Header size: %x\r\nEntry point: %x\r\nProgram header offset: %x\r\nSection header offset: %x\r\n", file_header_size, file_entry, file_program_header_offset, file_section_header_offset);

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
        //Kernel::KLog::the().printf("Segment %i: vaddr %x, file size %x, segment size %x, type %i\r\n", i, vaddr, size_in_file, size_in_memory, type);
        //Kernel::KLog::the().printf("Offset in file: %x\r\n", offset_in_file);
    
        // If this is a interpreter segment, read the interpreter out
        if(type == PT_INTERP) {
            if(isDynamic) { 
                Kernel::KLog::the().printf("ELF file error: several interperter sections\n\r");
                return false;
            }
            isDynamic = true;
            // Read out the path name
            interpreterPath = new char[size_in_file + 1];
            memset(interpreterPath, 0, size_in_file + 1);
            new_section->copy_out(interpreterPath);
            Kernel::KLog::the().printf("Interpreter: %s\n\r", interpreterPath);
        }
    }

    curr_pointer = file_section_header_offset;
    for(size_t i = 0; i < file_section_header_count; i++) {
        uint32_t sh_name = read32(curr_pointer); curr_pointer += 4;
        uint32_t sh_type = read32(curr_pointer); curr_pointer += 4;
        uint64_t sh_flags = read64(curr_pointer); curr_pointer += 8;
        uint64_t sh_addr = read64(curr_pointer); curr_pointer += 8;
        uint64_t sh_offset = read64(curr_pointer); curr_pointer += 8;
        uint64_t sh_size = read64(curr_pointer); curr_pointer += 8;
        uint32_t sh_link = read32(curr_pointer); curr_pointer += 4;
        uint32_t sh_info = read32(curr_pointer); curr_pointer += 4;
        uint64_t sh_addralign = read64(curr_pointer); curr_pointer += 8;
        uint64_t sh_entsize = read64(curr_pointer); curr_pointer += 8;
        
        SectionHeader* section_header = new SectionHeader;
        section_header->file_begin = sh_offset;
        section_header->size = sh_size;
        section_header->type = sh_type;
        section_header->link = sh_link;
        section_header->info = sh_info;
        section_header->ent_size = sh_entsize;
        section_headers.push_back(section_header);

        (void)sh_name;
        (void)sh_flags;
        (void)sh_addr;
        (void)sh_addralign;
    }
    return true;
}

void ELF::relocate(void* load_base) {
    for(size_t i = 0; i < section_headers.size(); i++) {
        SectionHeader* relocation = section_headers.at(i);
        // link must be valid
        if(relocation->link >= section_headers.size()) { continue; }
        // SymbolTable* symbol_table = section_headers.at(relocation->link)->toSymbols(data);
        
        if(relocation->type == SHT_RELA) {
            Kernel::KLog::the().printf("Relocation section RELA, section %i, link %i\n\r", i, relocation->link);
            relocateSingleTable(relocation->toRela(data), relocation, load_base);
        } else if(relocation->type == SHT_REL) {
            Kernel::KLog::the().printf("Relocation section REL, section %i, link %i\n\r", i, relocation->link);
            relocateSingleTable(relocation->toRel(data), relocation, load_base);    
        }
    }
    (void)load_base;
}

void ELF::relocateSingleTable(Rela* rela, SectionHeader* relocation, void* load_base) {
    SectionHeader* target = section_headers.at(relocation->info);
    for(size_t i = 0; i < relocation->size / relocation->ent_size; i++) {
        //uint64_t addr = (uint64_t)data + target->file_begin;
        uint64_t addr = (uint64_t)load_base + target->file_begin;
        uint64_t* ref = (uint64_t*)(addr + rela[i].r_offset);

        // We now read the symbol value, if we have one
        uint64_t symbol_value = 0;
        if(ELF64_R_SYM(rela[i].r_info) != SHN_UNDEF) {
            Kernel::KLog::the().printf("TODO: support symbols during relocation\n\r");
        }

        // We can now relocate based on type
        switch(ELF64_R_TYPE(rela[i].r_info)) {
            case R_AMD64_NONE: break;
            case R_AMD64_RELATIVE: {
                *ref = symbol_value + (uint64_t)(load_base) + rela[i].r_addend;
                break;
            }
            default: {
                Kernel::KLog::the().printf("TODO: support more relocation types; elf has type %i\n\r", ELF64_R_TYPE(rela[i].r_info));
            }
        }
    }
}

void ELF::relocateSingleTable(Rel* rel, SectionHeader* relocation, void* load_base) {
    SectionHeader* target = section_headers.at(relocation->info);
    for(size_t i = 0; i < relocation->size / relocation->ent_size; i++) {
        //uint64_t addr = (uint64_t)data + target->file_begin;
        uint64_t addr = (uint64_t)load_base + target->file_begin;
        uint64_t* ref = (uint64_t*)(addr + rel[i].r_offset);

        // We now read the symbol value, if we have one
        uint64_t symbol_value = 0;
        if(ELF64_R_SYM(rel[i].r_info) != SHN_UNDEF) {
            Kernel::KLog::the().printf("TODO: support symbols during relocation\n\r");
        }

        // We can now relocate based on type
        switch(ELF64_R_TYPE(rel[i].r_info)) {
            case R_AMD64_NONE: break;
            case R_AMD64_RELATIVE: {
                *ref = symbol_value + (uint64_t)(load_base);
                break;
            }
            default: {
                Kernel::KLog::the().printf("TODO: support more relocation types; elf has type %i\n\r", ELF64_R_TYPE(rel[i].r_info));
            }
        }
    }
}
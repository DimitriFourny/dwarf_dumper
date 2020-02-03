#include "ElfFile.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ElfFile::ElfFile(std::string filepath, bool& success) :
  memfile_(nullptr), debug_info_(nullptr), debug_info_size_(0),
  debug_abbrev_(nullptr), debug_abbrev_size_(0) {
  // Copy the file in memory
  FILE* hfile = fopen(filepath.c_str(), "rb");
  if (!hfile) {
    fprintf(stderr, "ERR: Failed to open '%s'\n", filepath.c_str());
    success = false;
    return;
  }
  success = true;

  fseek(hfile, 0, SEEK_END);
  filesize_ = ftell(hfile);
  fseek(hfile, 0, SEEK_SET);
  DBG_PRINTF("Target file size: 0x%lx\n", filesize_);

  memfile_ = reinterpret_cast<unsigned char*>(malloc(filesize_));
  size_t nb_read = fread(memfile_, filesize_, 1, hfile);
  fclose(hfile);
  if (nb_read != 1) {
    fprintf(stderr, "ERR: Failed to read '%s'\n", filepath.c_str());
    success = false;
    return;
  }

  // Save the headers pointers
  file_header_ = reinterpret_cast<Elf64_Ehdr*>(memfile_);
  program_header_ = reinterpret_cast<Elf64_Phdr*>(memfile_ 
                                                  + file_header_->e_phoff);
  section_header_ = reinterpret_cast<Elf64_Shdr*>(memfile_ 
                                                  + file_header_->e_shoff);

  char* names = reinterpret_cast<char*>(memfile_
                        + section_header_[file_header_->e_shstrndx].sh_offset);

  // Search the .debug_info and .debug_abbrev
  for (int i = 0; i < file_header_->e_shnum; i++) {
    char* name = &names[section_header_[i].sh_name];
    if (!strcmp(name, ".debug_info")) {
      debug_info_ = reinterpret_cast<unsigned char*>(memfile_ 
                                                + section_header_[i].sh_offset);
      debug_info_size_ = section_header_[i].sh_size;
    } else if (!strcmp(name, ".debug_abbrev")) {
      debug_abbrev_ = reinterpret_cast<unsigned char*>(memfile_ 
                                                + section_header_[i].sh_offset);
      debug_abbrev_size_ = section_header_[i].sh_size;
    } else if (!strcmp(name, ".debug_str")) {
      debug_str_ = reinterpret_cast<char*>(memfile_ 
                                          + section_header_[i].sh_offset);
      debug_str_size_ = section_header_[i].sh_size;
    }
  }

  success = (debug_info_ && debug_abbrev_);
}

ElfFile::~ElfFile() {
  if (memfile_) {
    free(memfile_);
  }
}

// static
uint32_t ElfFile::ULEB128(unsigned char* &data, size_t& bytes_available) {
  uint32_t result = 0;

  unsigned int shift = 0;
  while (bytes_available > 0) {
    unsigned char byte = *data;
    data++;
    bytes_available--;

    if (byte < 0x80) {
      result += byte << (shift*8);
      return result;
    } else {
      byte -= 0x80;
      result += byte << (shift*8);
    }

    shift++;
  }

  return result;
}

// static
void ElfFile::PassData(Dwarf32::Form form, unsigned char* &data, 
                                                      size_t& bytes_available) {
  uint32_t length = 0;

  switch(form) {
    // Address
    case Dwarf32::Form::DW_FORM_addr:
      data += sizeof(uint64_t);
      bytes_available -= sizeof(uint64_t);
        break;

    // Block
    case Dwarf32::Form::DW_FORM_block:
      length = ElfFile::ULEB128(data, bytes_available);
      data += length;
      bytes_available -= length;
      break;
    case Dwarf32::Form::DW_FORM_block1:
      length = *reinterpret_cast<uint8_t*>(data);
      data += sizeof(uint8_t) + length;
      bytes_available -= sizeof(uint8_t) + length;
      break;
    case Dwarf32::Form::DW_FORM_block2:
      length = *reinterpret_cast<uint16_t*>(data);
      data += sizeof(uint16_t) + length;
      bytes_available -= sizeof(uint16_t) + length;
      break;
    case Dwarf32::Form::DW_FORM_block4:
      length = *reinterpret_cast<uint32_t*>(data);
      data += sizeof(uint32_t) + length;
      bytes_available -= sizeof(uint32_t) + length;
      break;

    // Constant
    case Dwarf32::Form::DW_FORM_data1:
      data++;
      bytes_available--;
      break;
    case Dwarf32::Form::DW_FORM_data2:
      data += 2;
      bytes_available -= 2;
      break;
    case Dwarf32::Form::DW_FORM_data4:
      data += 4;
      bytes_available -= 4;
      break;
    case Dwarf32::Form::DW_FORM_data8:
      data += 8;
      bytes_available -= 8;
      break;
    case Dwarf32::Form::DW_FORM_sdata:
      ElfFile::ULEB128(data, bytes_available);
      break;
    case Dwarf32::Form::DW_FORM_udata:
      ElfFile::ULEB128(data, bytes_available);
      break;

    // Expression or location
    case Dwarf32::Form::DW_FORM_exprloc:
      length = ElfFile::ULEB128(data, bytes_available);
      data += length;
      bytes_available -= length;
      break;

    // Line offset
    case Dwarf32::Form::DW_FORM_sec_offset:
      data += 4;
      bytes_available -= 4;
      break;

    // Flag
    case Dwarf32::Form::DW_FORM_flag:
      data++;
      bytes_available--;
      break;
    case Dwarf32::Form::DW_FORM_flag_present:
      break;

    // Reference
    case Dwarf32::Form::DW_FORM_ref1:
      data++;
      bytes_available--;
      break;
    case Dwarf32::Form::DW_FORM_ref2:
      data += 2;
      bytes_available -= 2;
      break;
    case Dwarf32::Form::DW_FORM_ref4:
      data += 4;
      bytes_available -= 4;
      break;
    case Dwarf32::Form::DW_FORM_ref8:
      data += 8;
      bytes_available -= 8;
      break;
    case Dwarf32::Form::DW_FORM_ref_udata:
      ElfFile::ULEB128(data, bytes_available);
      break;
    case Dwarf32::Form::DW_FORM_ref_addr:
      data += 4;
      bytes_available -= 4;
      break;
    case Dwarf32::Form::DW_FORM_ref_sig8:
      data += 8;
      bytes_available -= 8;
      break;

    // String
    case Dwarf32::Form::DW_FORM_strp:
      data += 4;
      bytes_available -= 4;
      break;
    case Dwarf32::Form::DW_FORM_string:
      while (*data) {
          data++;
          bytes_available--;
      }
      data++;
      bytes_available--;
      break;

    default:
      fprintf(stderr, "ERR: Unpexpected form type 0x%x\n", form);
      break;
  }
}

uint64_t ElfFile::FormDataValue(Dwarf32::Form form, unsigned char* &info, 
                                                      size_t& bytes_available) {
  uint64_t value = 0;

  switch(form) {
    case Dwarf32::Form::DW_FORM_data1:
    case Dwarf32::Form::DW_FORM_ref1:
    case Dwarf32::Form::DW_FORM_flag_present:
      value = *reinterpret_cast<uint8_t*>(info);
      info++;
      bytes_available--;
      break;
    case Dwarf32::Form::DW_FORM_data2:
    case Dwarf32::Form::DW_FORM_ref2:
      value = *reinterpret_cast<uint16_t*>(info);
      info += 2;
      bytes_available -= 2;
      break;
    case Dwarf32::Form::DW_FORM_data4:
    case Dwarf32::Form::DW_FORM_ref4:
    case Dwarf32::Form::DW_FORM_ref_addr:
    case Dwarf32::Form::DW_FORM_sec_offset:
      value = *reinterpret_cast<uint32_t*>(info);
      info += 4;
      bytes_available -= 4;
      break;
    case Dwarf32::Form::DW_FORM_data8:
    case Dwarf32::Form::DW_FORM_ref8:
    case Dwarf32::Form::DW_FORM_ref_sig8:
      value = *reinterpret_cast<uint64_t*>(info);
      info += 8;
      bytes_available -= 8;
      break;
    case Dwarf32::Form::DW_FORM_sdata:
    case Dwarf32::Form::DW_FORM_udata:
    case Dwarf32::Form::DW_FORM_ref_udata:
    case Dwarf32::Form::DW_FORM_indirect:
      value = ElfFile::ULEB128(info, bytes_available);
      break;
    case Dwarf32::Form::DW_FORM_exprloc:
      value = ElfFile::ULEB128(info, bytes_available);
      info += value;
      bytes_available -= value;
      break;
    default:
      fprintf(stderr, "ERR: Unexpected form data 0x%x\n", form);
      exit(1);
  }

  return value;
};

char* ElfFile::FormStringValue(Dwarf32::Form form, unsigned char* &info, 
                                                      size_t& bytes_available) {
  char* str = nullptr;
  uint32_t str_pos = 0;

  switch(form) {
    case Dwarf32::Form::DW_FORM_strp:
      str_pos = *reinterpret_cast<uint32_t*>(info);
      info += sizeof(str_pos);
      bytes_available -= sizeof(str_pos);
      str = &debug_str_[str_pos];
      break;
    case Dwarf32::Form::DW_FORM_string:
      str = reinterpret_cast<char*>(info);
      while (*info) {
          info++;
          bytes_available--;
      }
      info++;
      bytes_available--;
      break;
    default:
      fprintf(stderr, "ERR: Unexpected form string 0x%x\n", form);
      break;
  }

  return str;
};

bool ElfFile::LoadAbbrevTags(uint32_t abbrev_offset) {
  if (!debug_info_ || !debug_abbrev_) {
      return false;
  }
  compilation_unit_.clear();

  unsigned char* abbrev = reinterpret_cast<unsigned char*>(debug_abbrev_ 
                                                          + abbrev_offset);
  size_t abbrev_bytes = debug_abbrev_size_ - abbrev_offset;

  // For all compilation tags
  while (abbrev_bytes > 0 && abbrev[0]) {
    struct TagSection section;
    section.number = ElfFile::ULEB128(abbrev, abbrev_bytes);
    DBG_PRINTF(".abbrev+%lx\t Tag Number %d\n",
        abbrev - debug_abbrev_, section.number);
    section.type =
        static_cast<Dwarf32::Tag>(ElfFile::ULEB128(abbrev, abbrev_bytes));
    section.has_children = *abbrev;
    abbrev++;
    abbrev_bytes--;
    section.ptr = abbrev;

    if (compilation_unit_.find(section.number) != compilation_unit_.end()) {
        fprintf(stderr, "ERR: Section number %d already exists\n", section.number);
        compilation_unit_.clear();
        return false;
    }
    compilation_unit_[section.number] = section;

    while (abbrev_bytes > 0 && abbrev[0]) { // For all attributes
        abbrev++;
    }
    abbrev += 2;
    abbrev_bytes -= 2;
  }

  return true;
}

#define CASE_REGISTER_NEW_TAG(tag_type, element_type)                         \
  case Dwarf32::Tag::tag_type:                                                \
    tree_builder_.AddElement(TreeBuilder::ElementType::element_type, tag_id); \
    break;

void ElfFile::RegisterNewTag(Dwarf32::Tag tag, uint64_t tag_id) {
  switch (tag) {
    CASE_REGISTER_NEW_TAG(DW_TAG_array_type, array_type)
    CASE_REGISTER_NEW_TAG(DW_TAG_class_type, class_type)
    CASE_REGISTER_NEW_TAG(DW_TAG_enumeration_type, enumerator_type)
    CASE_REGISTER_NEW_TAG(DW_TAG_member, member)
    CASE_REGISTER_NEW_TAG(DW_TAG_pointer_type, pointer_type)
    CASE_REGISTER_NEW_TAG(DW_TAG_structure_type, structure_type)
    CASE_REGISTER_NEW_TAG(DW_TAG_typedef, typedef2)
    CASE_REGISTER_NEW_TAG(DW_TAG_union_type, union_type)
    CASE_REGISTER_NEW_TAG(DW_TAG_inheritance, inheritance)
    CASE_REGISTER_NEW_TAG(DW_TAG_subrange_type, subrange_type)
    CASE_REGISTER_NEW_TAG(DW_TAG_base_type, base_type)
    CASE_REGISTER_NEW_TAG(DW_TAG_const_type, const_type)
    default:
      tree_builder_.AddNone();
  }
}

bool ElfFile::LogDwarfInfo(Dwarf32::Tag tag, Dwarf32::Attribute attribute, 
                uint64_t tag_id, Dwarf32::Form form, unsigned char* &info, 
                size_t& info_bytes, void* unit_base) {
  switch(attribute) {
    // Name
    case Dwarf32::Attribute::DW_AT_name:
    case Dwarf32::Attribute::DW_AT_linkage_name: {
      char* name = FormStringValue(form, info, info_bytes);
      tree_builder_.SetElementName(name);
      return true;
    }

    // Size
    case Dwarf32::Attribute::DW_AT_byte_size: {
      uint64_t byte_size = FormDataValue(form, info, info_bytes);
      tree_builder_.SetElementSize(byte_size);
      return true;
    }

    // Offset
    case Dwarf32::Attribute::DW_AT_data_member_location: {
      uint64_t offset = FormDataValue(form, info, info_bytes);
      tree_builder_.SetElementOffset(offset);
      return true;
    }

    // Type
    case Dwarf32::Attribute::DW_AT_type: {
      uint64_t id = FormDataValue(form, info, info_bytes);
      if (form != Dwarf32::Form::DW_FORM_ref_addr) {
        // The offset is relative to the current compilation unit, we make it
        // absolute
        id += reinterpret_cast<unsigned char*>(unit_base) - debug_info_;
      }
      tree_builder_.SetElementType(id);
      return true;
    }

    // Count
    case Dwarf32::Attribute::DW_AT_count: {
      uint64_t count = FormDataValue(form, info, info_bytes);
      tree_builder_.SetElementCount(count);
      return true;
    }

    default:
      return false;
  }

  return false;
}

bool ElfFile::GetAllClasses() {
  unsigned char* info = reinterpret_cast<unsigned char*>(debug_info_);
  size_t info_bytes = debug_info_size_;

  while (info_bytes > 0) {
    // Load the compilation unit information
    Dwarf32::CompilationUnitHdr* unit_hdr =
        reinterpret_cast<Dwarf32::CompilationUnitHdr*>(info);
    DBG_PRINTF("unit_length         = 0x%x\n", unit_hdr->unit_length);
    DBG_PRINTF("version             = %d\n", unit_hdr->version);
    DBG_PRINTF("debug_abbrev_offset = 0x%x\n", unit_hdr->debug_abbrev_offset);
    DBG_PRINTF("address_size        = %d\n", unit_hdr->address_size);
    unsigned char* info_end = info + unit_hdr->unit_length + sizeof(uint32_t);
    info += sizeof(Dwarf32::CompilationUnitHdr);
    info_bytes -= sizeof(Dwarf32::CompilationUnitHdr);

    if (!LoadAbbrevTags(unit_hdr->debug_abbrev_offset)) {
      fprintf(stderr, "ERR: Can't load the compilation\n");
      return false;
    }

    // For all compilation tags
    while (info < info_end) {
      uint64_t tag_id = info - debug_info_; 
      uint32_t info_number = ElfFile::ULEB128(info, info_bytes);
      DBG_PRINTF(".info+%lx\t Info Number %d\n", info-debug_info_, info_number);
      if (!info_number) { // reserved
        continue;
      }

      std::map<unsigned int, struct TagSection>::iterator it_section =
          compilation_unit_.find(info_number);
      if (it_section == compilation_unit_.end()) {
        fprintf(stderr, "ERR: Can't find tag number %d\n", info_number);
        return false;
      }
      TagSection* section = &it_section->second;
      unsigned char* abbrev = section->ptr;
      size_t abbrev_bytes = debug_abbrev_size_ - (abbrev - debug_abbrev_);

      RegisterNewTag(section->type, tag_id);

      // For all attributes
      while (*abbrev) {
        Dwarf32::Attribute abbrev_attribute = static_cast<Dwarf32::Attribute>(
            ElfFile::ULEB128(abbrev, abbrev_bytes));
        Dwarf32::Form abbrev_form = 
            static_cast<Dwarf32::Form>(ElfFile::ULEB128(abbrev, abbrev_bytes));

        DBG_PRINTF(".info+%lx\t %02x %02x\n", info-debug_info_, 
                                                abbrev_attribute, abbrev_form);
        bool logged = LogDwarfInfo(section->type, abbrev_attribute, 
          tag_id, abbrev_form, info, info_bytes, unit_hdr);
        if (!logged) {
          ElfFile::PassData(abbrev_form, info, info_bytes);
        }
      }
    }
  }

  return false;
}



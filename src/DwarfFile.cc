#include "DwarfFile.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// TODO: add bounds check!


void DwarfFile::SetDebugPointers(void* debug_info, size_t debug_info_size, 
                                 void* debug_abbrev, size_t debug_abbrev_size, 
                                 void* debug_str, size_t debug_str_size) 
{
  debug_info_ = debug_info;
  debug_info_size_ = debug_info_size;
  debug_abbrev_ = debug_abbrev;
  debug_abbrev_size_ = debug_abbrev_size;
  debug_str_ = debug_str;
  debug_str_size_ = debug_str_size;
  is_loaded_ = true;
}

bool DwarfFile::IsValidFilePtr(void* ptr, size_t size) 
{
  const void* file_begin = memfile_;
  const void* file_end = memfile_ + filesize_;
  const char* cptr = reinterpret_cast<char*>(ptr);

  if (cptr + size < cptr) {
    return false;           // Overflow
  }
  return (cptr >= file_begin) && (cptr + size < file_end);
}

// static
uint32_t DwarfFile::ULEB128(unsigned char* &data, size_t& bytes_available) 
{
  uint32_t result = 0;

  unsigned int shift = 0;
  while (bytes_available > 0) {
    unsigned char byte = *data;
    data++;
    bytes_available--;

    if (byte < 0x80) {
      result += byte << (shift*7);
      return result;
    } else {
      byte -= 0x80;
      result += byte << (shift*7);
    }

    shift++;
  }

  return result;
}

// static
void DwarfFile::PassData(Dwarf32::Form form, unsigned char* &data, size_t& bytes_available) 
{
  uint32_t length = 0;

  switch(form) {
    // Address
    case Dwarf32::Form::DW_FORM_addr:
      data += sizeof(uint64_t);
      bytes_available -= sizeof(uint64_t);
      break;

    // Block
    case Dwarf32::Form::DW_FORM_block:
      length = DwarfFile::ULEB128(data, bytes_available);
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
      DwarfFile::ULEB128(data, bytes_available);
      break;
    case Dwarf32::Form::DW_FORM_udata:
      DwarfFile::ULEB128(data, bytes_available);
      break;

    // Expression or location
    case Dwarf32::Form::DW_FORM_exprloc:
      length = DwarfFile::ULEB128(data, bytes_available);
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
      DwarfFile::ULEB128(data, bytes_available);
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

uint64_t DwarfFile::FormDataValue(Dwarf32::Form form, unsigned char* &info, size_t& bytes_available) 
{
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
      value = DwarfFile::ULEB128(info, bytes_available);
      break;
    case Dwarf32::Form::DW_FORM_exprloc:
      value = DwarfFile::ULEB128(info, bytes_available);
      info += value;
      bytes_available -= value;
      break;
    default:
      fprintf(stderr, "ERR: Unexpected form data 0x%x\n", form);
      exit(1);
  }

  return value;
};

char* DwarfFile::FormStringValue(Dwarf32::Form form, unsigned char* &info, size_t& bytes_available) 
{
  char* str = nullptr;
  uint32_t str_pos = 0;

  switch(form) {
    case Dwarf32::Form::DW_FORM_strp:
      str_pos = *reinterpret_cast<uint32_t*>(info);
      info += sizeof(str_pos);
      bytes_available -= sizeof(str_pos);
      str = reinterpret_cast<char*>(debug_str_) + str_pos;
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

bool DwarfFile::LoadAbbrevTags(uint32_t abbrev_offset) 
{
  // TODO: for optimization, if we have previously parsed the same abbrev_offset, don't parse it again and just
  //       don't clear compilation_unit_.
  compilation_unit_.clear();

  unsigned char* abbrev = reinterpret_cast<unsigned char*>(debug_abbrev_) + abbrev_offset;
  size_t abbrev_bytes = debug_abbrev_size_ - abbrev_offset;

  // For all compilation tags
  while (abbrev_bytes > 0) {
    struct TagSection section;
    section.number = DwarfFile::ULEB128(abbrev, abbrev_bytes);
    if (section.number == 0) {
      // End of the tags list
      break;
    }

    // DBG_PRINTF(".abbrev+%lx\t Tag Number %d\n", 
    //     abbrev - reinterpret_cast<unsigned char*>(debug_abbrev_), section.number);

    section.type = static_cast<Dwarf32::Tag>(DwarfFile::ULEB128(abbrev, abbrev_bytes));
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

  // DBG_PRINTF("compilation_unit_.size()  = %lu\n", compilation_unit_.size());
  return true;
}

#define CASE_REGISTER_NEW_TAG(tag_type, element_type)                                       \
  case Dwarf32::Tag::tag_type:                                                              \
    tree_builder_.AddElement(TreeBuilder::ElementType::element_type, tag_id, has_children); \
    break;

void DwarfFile::RegisterNewTag(Dwarf32::Tag tag, uint64_t tag_id, bool has_children) {
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
      tree_builder_.AddElement(TreeBuilder::ElementType::none, 0, has_children);
  }
}

bool DwarfFile::LogDwarfInfo(
    Dwarf32::Tag tag, Dwarf32::Attribute attribute,  uint64_t tag_id, Dwarf32::Form form, unsigned char* &info, 
    size_t& info_bytes, void* unit_base) 
{
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
        id += reinterpret_cast<char*>(unit_base) - reinterpret_cast<char*>(debug_info_);
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

bool DwarfFile::GetAllClasses() 
{
  if (!is_loaded_) {
    return false;
  }
  
  unsigned char* info = reinterpret_cast<unsigned char*>(debug_info_);
  size_t info_bytes = debug_info_size_;

  while (info_bytes > 0) {
    // Load the compilation unit information
    Dwarf32::CompilationUnitHdr* unit_hdr =
        reinterpret_cast<Dwarf32::CompilationUnitHdr*>(info);
    DBG_PRINTF("\nunit offset   = 0x%lx\n", info - reinterpret_cast<unsigned char*>(debug_info_));
    DBG_PRINTF("unit_length   = 0x%x\n", unit_hdr->unit_length);
    DBG_PRINTF("version       = %d\n", unit_hdr->version);
    DBG_PRINTF("abbrev_offset = 0x%x\n", unit_hdr->abbrev_offset);
    DBG_PRINTF("address_size  = %d\n", unit_hdr->address_size);
    unsigned char* info_end = info + unit_hdr->unit_length + sizeof(uint32_t);
    info += sizeof(Dwarf32::CompilationUnitHdr);
    info_bytes -= sizeof(Dwarf32::CompilationUnitHdr);

    if (!LoadAbbrevTags(unit_hdr->abbrev_offset)) {
      fprintf(stderr, "ERR: Can't load the compilation\n");
      return false;
    }

    // For all compilation tags
    int depth = 0;
    while (info < info_end) {
      uint64_t tag_id = info - reinterpret_cast<unsigned char*>(debug_info_); 
      uint32_t abbrev_num = DwarfFile::ULEB128(info, info_bytes);

      // if (tag_id >= 0x0ab78256 && tag_id < 0x0ab7835d) {
      DBG_PRINTF(".info+%lx\t Tag 0x%lx ; Info Number %d\n", info-reinterpret_cast<unsigned char*>(debug_info_), tag_id, abbrev_num);
      // }

      if (!abbrev_num) { // Null DIE so end of the children list
        tree_builder_.EndOfChildren();
        depth--;
        continue;
      }

      std::map<unsigned int, struct TagSection>::iterator it_section = compilation_unit_.find(abbrev_num);
      if (it_section == compilation_unit_.end()) {
        fprintf(stderr, "ERR at 0x%lx: Can't find compilation unit with abbrev number %d\n", 
            info-reinterpret_cast<unsigned char*>(debug_info_), abbrev_num);
        return false;
      }
      TagSection* section = &it_section->second;
      unsigned char* abbrev = section->ptr;
      size_t abbrev_bytes = debug_abbrev_size_ - (abbrev - reinterpret_cast<unsigned char*>(debug_abbrev_));

      // if (tag_id >= 0x0ab78256 && tag_id < 0x0ab7835d) {
      DBG_PRINTF("[%d] section->num = %d; section->type = 0x%x ; has_children = %d\n", depth, section->number, section->type, section->has_children);
      // }

      // Register the new tag (class, structure, namespace, etc.)
      RegisterNewTag(section->type, tag_id, section->has_children);

      // Increment the depth for the next children 
      if (section->has_children) {
        depth++;
      }

      // For all attributes
      while (true) {
        Dwarf32::Attribute abbrev_attribute = static_cast<Dwarf32::Attribute>(DwarfFile::ULEB128(abbrev, abbrev_bytes));
        Dwarf32::Form abbrev_form = static_cast<Dwarf32::Form>(DwarfFile::ULEB128(abbrev, abbrev_bytes));
        if (!abbrev_attribute && !abbrev_form) {
          // End of the attribute list
          break;
        }

        if (tag_id >= 0x0ab78256 && tag_id < 0x0ab7835d) {
          DBG_PRINTF(".info+%lx\t %02x %02x\n", 
              info-reinterpret_cast<unsigned char*>(debug_info_), abbrev_attribute, abbrev_form);
        }

        bool logged = LogDwarfInfo(section->type, abbrev_attribute, tag_id, abbrev_form, info, info_bytes, unit_hdr);
        if (!logged) {
          DwarfFile::PassData(abbrev_form, info, info_bytes);
        }
      }
    }
  }

  return false;
}



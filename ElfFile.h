#pragma once
#include <string>
#include <elf.h>
#include <map>
#include <vector>
#include "dwarf32.h"
#include "TreeBuilder.h"


class ElfFile {
public:
  ElfFile(std::string filepath, bool& success);
  ~ElfFile();
  bool GetAllClasses();

  std::string json() { return tree_builder_.GenerateJson(); }

private:
  static uint32_t ULEB128(unsigned char* &data, size_t& bytes_available);
  static void PassData(Dwarf32::Form form,
      unsigned char* &data, size_t& bytes_available);
  uint64_t FormDataValue(Dwarf32::Form form,
      unsigned char* &info, size_t& bytes_available);
  char* FormStringValue(Dwarf32::Form form,
      unsigned char* &info, size_t& bytes_available);
  bool LoadAbbrevTags(uint32_t abbrev_offset);
  void RegisterNewTag(Dwarf32::Tag tag, uint64_t tag_id);
  bool LogDwarfInfo(Dwarf32::Tag tag, Dwarf32::Attribute attribute, 
    uint64_t tag_id, Dwarf32::Form form, unsigned char* &info, 
    size_t& info_bytes, void* unit_base);

  size_t filesize_;
  unsigned char* memfile_;

  Elf64_Ehdr* file_header_;
  Elf64_Phdr* program_header_;
  Elf64_Shdr* section_header_;

  unsigned char* debug_info_;
  size_t debug_info_size_;
  unsigned char* debug_abbrev_;
  size_t debug_abbrev_size_;
  char* debug_str_;
  size_t debug_str_size_;

  struct TagSection {
      unsigned int number;
      Dwarf32::Tag type;
      bool has_children;
      unsigned char* ptr;
  };
  typedef std::map<unsigned int, struct TagSection> CompilationUnit;
  CompilationUnit compilation_unit_;

  TreeBuilder tree_builder_;
};

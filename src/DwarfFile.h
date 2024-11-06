#pragma once
#include <string>
#include <map>
#include <vector>
#include "dwarf32.h"
#include "TreeBuilder.h"


class DwarfFile {
public:
  DwarfFile() : memfile_(0), is_loaded_(false) {};
  ~DwarfFile() = default;

  void SetDebugPointers(void* debug_info, size_t debug_info_size, 
                        void* debug_abbrev, size_t debug_abbrev_size, 
                        void* debug_str, size_t debug_str_size);

  bool GetAllClasses();
  std::string json() { return tree_builder_.GenerateJson(); }


protected:
  size_t filesize_;
  unsigned char* memfile_;
  bool is_loaded_;

  bool IsValidFilePtr(void* ptr, size_t size = 0);


private:
  static uint32_t ULEB128(unsigned char* &data, size_t& bytes_available);
  static void PassData(Dwarf32::Form form, unsigned char* &data, size_t& bytes_available);
  uint64_t FormDataValue(Dwarf32::Form form, unsigned char* &info, size_t& bytes_available);
  char* FormStringValue(Dwarf32::Form form, unsigned char* &info, size_t& bytes_available);
  bool LoadAbbrevTags(uint32_t abbrev_offset);
  void RegisterNewTag(Dwarf32::Tag tag, uint64_t tag_id, bool has_children);
  bool LogDwarfInfo(Dwarf32::Tag tag, Dwarf32::Attribute attribute,  uint64_t tag_id, Dwarf32::Form form, 
                    unsigned char* &info, size_t& info_bytes, void* unit_base);

  void* debug_info_;
  size_t debug_info_size_;
  void* debug_abbrev_;
  size_t debug_abbrev_size_;
  void* debug_str_;
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

#include "ElfFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"
#include "debug.h"


ElfFile::~ElfFile() {
  if (memfile_) {
    free(memfile_);
  }
}

bool ElfFile::Load(std::string filepath) 
{
  if (is_loaded_) {
    free(memfile_);
    memfile_ = nullptr;
  }
  is_loaded_ = false;

  // Copy the file in memory
  FILE* hfile = fopen(filepath.c_str(), "rb");
  if (!hfile) {
    fprintf(stderr, "ERR: Failed to open '%s'\n", filepath.c_str());
    return false;
  }

  fseek(hfile, 0, SEEK_END);
  filesize_ = ftell(hfile);
  fseek(hfile, 0, SEEK_SET);
  DBG_PRINTF("Target file size: 0x%lx\n", filesize_);

  memfile_ = reinterpret_cast<unsigned char*>(malloc(filesize_));
  size_t nb_read = fread(memfile_, filesize_, 1, hfile);
  fclose(hfile);
  if (nb_read != 1) {
    fprintf(stderr, "ERR: Failed to read '%s'\n", filepath.c_str());
    return false;
  }

  // Get the headers pointers
  Elf64_Ehdr* file_header_ = reinterpret_cast<Elf64_Ehdr*>(memfile_);
  if (!IsValidFilePtr(file_header_)) {
    fprintf(stderr, "ERR: Invalid file header\n");
    return false;
  } 

  Elf64_Phdr* program_header_ = reinterpret_cast<Elf64_Phdr*>(memfile_ + file_header_->e_phoff);
  if (!IsValidFilePtr(program_header_)) {
    fprintf(stderr, "ERR: Invalid program header\n");
    return false;
  }

  Elf64_Shdr* section_header_ = reinterpret_cast<Elf64_Shdr*>(memfile_ + file_header_->e_shoff);
  if (!IsValidFilePtr(section_header_)) {
    fprintf(stderr, "ERR: Invalid section header\n");
    return false;
  }

  char* names = reinterpret_cast<char*>(memfile_ + section_header_[file_header_->e_shstrndx].sh_offset);
  if (!IsValidFilePtr(names)) {
    return false;
  }

  // Search the debug sections
  void* debug_info = 0;
  size_t debug_info_size = 0;
  void* debug_abbrev = 0;
  size_t debug_abbrev_size = 0;
  void* debug_str = 0;
  size_t debug_str_size = 0;

  for (int i = 0; i < file_header_->e_shnum; i++) {
    char* name = &names[section_header_[i].sh_name];
    if (!strcmp(name, ".debug_info")) {
      debug_info = reinterpret_cast<unsigned char*>(memfile_ + section_header_[i].sh_offset);
      debug_info_size = section_header_[i].sh_size;
    } else if (!strcmp(name, ".debug_abbrev")) {
      debug_abbrev = reinterpret_cast<unsigned char*>(memfile_ + section_header_[i].sh_offset);
      debug_abbrev_size = section_header_[i].sh_size;
    } else if (!strcmp(name, ".debug_str")) {
      debug_str = reinterpret_cast<char*>(memfile_ + section_header_[i].sh_offset);
      debug_str_size = section_header_[i].sh_size;
    }
  }

  if (!IsValidFilePtr(debug_info, debug_info_size) || !IsValidFilePtr(debug_abbrev, debug_abbrev_size) || 
      !IsValidFilePtr(debug_str, debug_str_size)) {
    return false;
  }

  SetDebugPointers(debug_info, debug_info_size, debug_abbrev, debug_abbrev_size, debug_str, debug_str_size);
  return true;
}
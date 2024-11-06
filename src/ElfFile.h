#pragma once
#include "DwarfFile.h"


class ElfFile : public DwarfFile {
public:
  ElfFile() = default;
  ~ElfFile();
  bool Load(std::string filepath);
};

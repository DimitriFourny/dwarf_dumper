#pragma once
#include "DwarfFile.h"


class MachOFile : public DwarfFile {
public:
  MachOFile() = default;
  ~MachOFile();
  bool Load(std::string filepath, std::string target_arch);
};

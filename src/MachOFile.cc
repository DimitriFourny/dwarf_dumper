#include "MachOFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macho.h"
#include "debug.h"

#ifdef __MACH__
#include <mach/machine.h>
#else 
#include <arpa/inet.h>
typedef int	cpu_type_t;
typedef int	cpu_subtype_t;
#define CPU_TYPE_I386		    ((cpu_type_t) 7)
#define CPU_TYPE_ARM		    ((cpu_type_t) 12)
#define CPU_ARCH_ABI64		  0x1000000
#define CPU_TYPE_ARM64		  ((cpu_type_t)(CPU_TYPE_ARM | CPU_ARCH_ABI64))
#define CPU_TYPE_X86_64		  ((cpu_type_t) (CPU_TYPE_I386 | CPU_ARCH_ABI64))
#define CPU_SUBTYPE_MASK    0xff000000
#define	CPU_SUBTYPE_ARM64E  ((cpu_subtype_t) 2)
#endif

MachOFile::~MachOFile() {
  if (memfile_) {
    free(memfile_);
  }
}

bool MachOFile::Load(std::string filepath, std::string target_arch) 
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

  // Load commands
  mach_header_64* header = reinterpret_cast<mach_header_64*>(memfile_);
  if (!IsValidFilePtr(header, sizeof(*header))) {
    return false;
  }

  if (header->magic == 0xbebafeca) {
    // Fat Mach-O and the user don't want to extract the binary with the command
    // lipo -thin arm64e ./myfile -output ./myfile_arm64e
    header = nullptr;

    fat_header* fheader = reinterpret_cast<fat_header*>(memfile_);
    uint32_t nfat_arch = ntohl(fheader->nfat_arch);

    uint32_t target_cputype = 0;
    uint32_t target_cpusubtype = 0;

    if (target_arch == "arm64") {
      target_cputype = CPU_TYPE_ARM64;
    } else if (target_arch == "arm64e") {
      target_cputype = CPU_TYPE_ARM64;
      target_cpusubtype = CPU_SUBTYPE_ARM64E;
    } else if (target_arch == "x86_64") {
      target_cputype = CPU_TYPE_X86_64;
    } else {
      fprintf(stderr, "ERR: arch %s is not supported\n", target_arch.c_str());
      return false;
    }

    fat_arch* archs = reinterpret_cast<fat_arch*>(reinterpret_cast<char*>(fheader) + sizeof(*fheader));
    for (size_t i = 0; i < nfat_arch; i++) {
      if (!IsValidFilePtr(&archs[i], sizeof(fat_arch))) {
        return false;
      }

      uint32_t cputype = ntohl(archs[i].cputype);
      uint32_t cpusubtype = ntohl(archs[i].cpusubtype & CPU_SUBTYPE_MASK);

      if (cputype == target_cputype && (!target_cpusubtype || cpusubtype == target_cpusubtype)) {
        uint32_t offset = ntohl(archs[i].offset);
      
        header = reinterpret_cast<mach_header_64*>(memfile_ + offset);
        if (!IsValidFilePtr(header, sizeof(*header))) {
          return false;
        }
        break;
      }
    }

    if (!header) {
      fprintf(stderr, "ERR: arch %s not fond\n", target_arch.c_str());
      return false;
    }
  }

  if (header->magic != 0xfeedfacf) {
    fprintf(stderr, "magic 0x%x not supported\n", header->magic);
    return false;
  }

  // Search the DWARF segment
  segment_command_64* dwarf_seg = 0;
  command_header* cmd = reinterpret_cast<command_header*>(reinterpret_cast<char*>(header) + sizeof(*header));

  for (size_t i = 0; i < header->ncmds; i++) {
    if (!IsValidFilePtr(cmd, sizeof(*cmd))) {
      return false;
    }

    if (cmd->cmd == LC_SEGMENT_64) {
      segment_command_64* seg = reinterpret_cast<segment_command_64*>(cmd); 
      if (!IsValidFilePtr(seg, sizeof(*seg))) {
        return false;
      }

      if (!strcmp(reinterpret_cast<char*>(&seg->segname), "__DWARF")) {
        dwarf_seg = seg;
        break;
      }
    }

    cmd = reinterpret_cast<command_header*>(reinterpret_cast<char*>(cmd) + cmd->cmdsize);
  }

  if (!dwarf_seg) {
    return false;
  }

  // Search the debug sections
  void* debug_info = 0;
  size_t debug_info_size = 0;
  void* debug_abbrev = 0;
  size_t debug_abbrev_size = 0;
  void* debug_str = 0;
  size_t debug_str_size = 0;

  section_64* section = reinterpret_cast<section_64*>(reinterpret_cast<char*>(dwarf_seg) + sizeof(*dwarf_seg));

  for (size_t i = 0; i < dwarf_seg->nsects; i++) {
    if (!IsValidFilePtr(section, sizeof(*section))) {
      return false;
    }

    if (!strcmp(section->sectname, "__debug_info")) {
      debug_info = reinterpret_cast<char*>(header) + section->offset;
      debug_info_size = section->size;
    } else if (!strcmp(section->sectname, "__debug_abbrev")) {
      debug_abbrev = reinterpret_cast<char*>(header) + section->offset;
      debug_abbrev_size = section->size;
    } else if (!strcmp(section->sectname, "__debug_str")) {
      debug_str = reinterpret_cast<char*>(header) + section->offset;
      debug_str_size = section->size;
    }

    section++;
  }

  if (!IsValidFilePtr(debug_info, debug_info_size) || !IsValidFilePtr(debug_abbrev, debug_abbrev_size) || 
      !IsValidFilePtr(debug_str, debug_str_size)) {
    return false;
  }

  SetDebugPointers(debug_info, debug_info_size, debug_abbrev, debug_abbrev_size, debug_str, debug_str_size);
  return true;
}
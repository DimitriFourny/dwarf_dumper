// #include "MachOFile.h"
#include "ElfFile.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Format: %s <binary_path> [arm64e|arm64|x86_64]\n", argv[0]);
    return 1;
  }

  std::string target_arch = "arm64e";
  if (argc > 2) {
    target_arch = std::string(argv[2]);
  }

  std::string binary_path = std::string(argv[1]);
  ElfFile file;
  if (!file.Load(binary_path)) {
    fprintf(stderr, "Can't load the file\n");
    return 2;
  }

  file.GetAllClasses();
  std::string json = file.json();
  printf("%s\n", json.c_str());

  return 0;
}
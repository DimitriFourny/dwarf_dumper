#include "ElfFile.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Format: %s <binary_path>\n", argv[0]);
    return 1;
  }

  bool success;
  std::string binary_path = std::string(argv[1]);
  ElfFile file(binary_path, success);
  if (!success) {
    return 2;
  }

  file.GetAllClasses();
  std::string json = file.json();
  printf("%s\n", json.c_str());

  return 0;
}
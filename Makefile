BIN_NAME = dwarf_dumper
CXX = clang++
CXXFLAGS = -std=c++17 -Ofast -Wall
LIBS = 

SRC_FILES_CPP = $(wildcard src/*.cc)
OBJ_FILES = $(patsubst src/%.cc, build/%.o, $(SRC_FILES_CPP))


all: build/$(BIN_NAME)
	codesign -d -f -s - $<

build/:
	mkdir build

build/$(BIN_NAME): build/ $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) $(LIBS) -o build/$(BIN_NAME)

build/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

clean:
	-rm build/*


# Rebuild an object if an included header is modified
DEP_FILES = $(patsubst %.o, %.d, $(OBJ_FILES))
-include $(DEP_FILES)
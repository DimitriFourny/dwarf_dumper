all:
	mkdir -p bin
	g++ -O3 main.cc ElfFile.cc TreeBuilder.cc -o bin/dumper -Wall

debug:
	mkdir -p bin
	clang++ -O3 -g -fsanitize=address -fno-omit-frame-pointer main.cc ElfFile.cc TreeBuilder.cc -o bin/dumper -I rapidjson/include -Wall
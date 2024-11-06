all: dwarf_arm64 dwarf_arm64e dwarf_fat

dwarf_arm64:
	clang++ -std=c++14 -g main.cc b.cc -o dwarf_arm64
	mv dwarf_arm64.dSYM/Contents/Resources/DWARF/dwarf_arm64 dwarf_arm64
	rm -rf dwarf_arm64.dSYM

dwarf_arm64e:
	clang++ -arch arm64e -std=c++14 -g main.cc b.cc -o dwarf_arm64e
	mv dwarf_arm64e.dSYM/Contents/Resources/DWARF/dwarf_arm64e dwarf_arm64e
	rm -rf dwarf_arm64e.dSYM
	
dwarf_fat: dwarf_arm64 dwarf_arm64e
	lipo -create dwarf_arm64 dwarf_arm64e -o dwarf_fat

clean:
	-rm dwarf_fat dwarf_arm64e dwarf_arm64
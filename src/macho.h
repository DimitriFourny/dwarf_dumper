#pragma once

#include <stdint.h>

#define LC_SEGMENT  0x1
#define LC_SYMTAB 0x2
#define LC_SYMSEG 0x3
#define LC_THREAD 0x4
#define LC_UNIXTHREAD 0x5
#define LC_LOADFVMLIB 0x6
#define LC_IDFVMLIB 0x7
#define LC_IDENT  0x8
#define LC_FVMFILE  0x9
#define LC_PREPAGE 0xa
#define LC_DYSYMTAB 0xb
#define LC_LOAD_DYLIB 0xc
#define LC_ID_DYLIB 0xd
#define LC_LOAD_DYLINKER 0xe
#define LC_ID_DYLINKER  0xf
#define LC_PREBOUND_DYLIB 0x10
#define LC_ROUTINES 0x11
#define LC_SUB_FRAMEWORK 0x12 
#define LC_SUB_UMBRELLA 0x13 
#define LC_SUB_CLIENT 0x14
#define LC_SUB_LIBRARY  0x15
#define LC_TWOLEVEL_HINTS 0x16  
#define LC_PREBIND_CKSUM  0x17 

#define LC_LOAD_WEAK_DYLIB (0x18 | LC_REQ_DYLD)
#define LC_SEGMENT_64 0x19
#define LC_ROUTINES_64  0x1a 
#define LC_UUID   0x1b 
#define LC_RPATH (0x1c | LC_REQ_DYLD)   
#define LC_CODE_SIGNATURE 0x1d  
#define LC_SEGMENT_SPLIT_INFO 0x1e 
#define LC_REEXPORT_DYLIB (0x1f | LC_REQ_DYLD) 
#define LC_LAZY_LOAD_DYLIB 0x20 
#define LC_ENCRYPTION_INFO 0x21
#define LC_DYLD_INFO  0x22  
#define LC_DYLD_INFO_ONLY (0x22|LC_REQ_DYLD) 
#define LC_LOAD_UPWARD_DYLIB (0x23 | LC_REQ_DYLD) 
#define LC_VERSION_MIN_MACOSX 0x24   
#define LC_VERSION_MIN_IPHONEOS 0x25
#define LC_FUNCTION_STARTS 0x26
#define LC_DYLD_ENVIRONMENT 0x27 
#define LC_MAIN (0x28|LC_REQ_DYLD)
#define LC_DATA_IN_CODE 0x29
#define LC_SOURCE_VERSION 0x2A 
#define LC_DYLIB_CODE_SIGN_DRS 0x2B 
#define LC_BUILD_VERSION 0x32

#define S_ATTR_PURE_INSTRUCTIONS 0x80000000


typedef struct {
  uint32_t magic; 
  uint32_t cputype; 
  uint32_t cpusubtype; 
  uint32_t filetype;
  uint32_t ncmds;    
  uint32_t sizeofcmds; 
  uint32_t flags;
  uint32_t reserved;
} mach_header_64;

struct fat_header {
	uint32_t	magic;		 
	uint32_t	nfat_arch;
};

struct fat_arch {
	uint32_t	cputype;	    
	uint32_t	cpusubtype;	 
	uint32_t	offset;		 
	uint32_t	size;		  
	uint32_t	align;		
};

typedef struct {
  uint32_t cmd;       
  uint32_t cmdsize;   
} command_header;

typedef struct {
  uint32_t cmd;       
  uint32_t cmdsize;   
  char     segname[16]; 
  uint64_t vmaddr;    
  uint64_t vmsize;  
  uint64_t fileoff;     
  uint64_t filesize;     
  uint32_t maxprot;     
  uint32_t initprot;  
  uint32_t nsects;    
  uint32_t flags;
} segment_command_64;

typedef struct { 
	char		sectname[16];
	char		segname[16];
	uint64_t	addr; 
	uint64_t	size; 
	uint32_t	offset;
	uint32_t	align;
	uint32_t	reloff;
	uint32_t	nreloc;
	uint32_t	flags;
	uint32_t	reserved1;
	uint32_t	reserved2;
	uint32_t	reserved3;
} section_64;
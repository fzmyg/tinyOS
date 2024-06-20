#ifndef __EXEC_H__
#define __EXEC_H__
#include"stdint.h"

typedef uint32_t Elf32_Addr,Elf32_Word,Elf32_Off;
typedef uint16_t Elf32_Half;

struct Elf32_Ehdr{ //elf file head entry
    uint8_t e_ident[16]; //ELF魔数
    Elf32_Half e_type;   //ELF文件类型
    Elf32_Half e_machine;//机器类型
    Elf32_Word e_version;//ELF文件版本
    Elf32_Addr e_entry;  //程序入口点地址
    Elf32_Off  e_phroff; //程序头表在文件中偏移
    Elf32_Off  e_shroff; //节头表在文件中偏移
    Elf32_Word e_flags;  //与文件相关的特定处理器标识
    Elf32_Half e_ehsize; //ELF文件头大小
    Elf32_Half e_phrentsize;//程序头表条目大小
    Elf32_Half e_phrnum; //程序头表条目数量
    Elf32_Half e_shrentsize;//节头表条目大小
    Elf32_Half e_shrnum;  //节头表条目数量
    Elf32_Half e_shstrndx; //节头字符串表在节头表中的条目数量
};

struct Elf32_Phdr{ //program head table entry
    Elf32_Word p_type;  //段类型
    Elf32_Off  p_offset;//段在文件中的偏移
    Elf32_Addr p_vaddr; //段在内存中的虚拟地址
    Elf32_Addr p_paddr; //段在内存中的物理地址（无意义）
    Elf32_Word p_filesz;//段在文件中的大小
    Elf32_Word p_memsz; //段在内存中大小   (由与.bss段等使得内存空间大小实际大于文件中空间大小)
    Elf32_Word p_flags; //段的属性
    Elf32_Word p_align; //段在内存和文件中的对齐方式。值为 0 或 1 表示段不需要对齐；否则，必须是 2 的幂。加载程序将该段的起始地址对齐到该值。
};

enum segment_type{ //程序段类型
    PT_NULL, //空段
    PT_LOAD, //可加载段
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR
};

extern int32_t sys_execv(const char* file_path,char* argv[]);

#endif
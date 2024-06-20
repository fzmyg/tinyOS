#include"exec.h"
#include"process.h"
#include"fs.h"
#include"memory.h"
#include"string.h"
#include"thread.h"
#define DIV_ROUND_UP(a,b) (((a)-1)/(b)+1)
static int32_t loadSegment(struct Elf32_Phdr * prog_header,int fd)
{
    Elf32_Addr first_page_vaddr = prog_header->p_vaddr & 0xfffff000;
    uint32_t  first_page_size = PG_SIZE - (prog_header->p_vaddr & 0x00000fff);
    uint32_t occupy_page_cnt  = 0;
    if(prog_header->p_memsz>first_page_size){
        occupy_page_cnt = DIV_ROUND_UP(prog_header->p_memsz-first_page_size,PG_SIZE)+1;
    }else{
        occupy_page_cnt = 1;
    }
    uint32_t vaddr = first_page_vaddr;
    uint32_t i = 0;
    for(;i<occupy_page_cnt;i++){
        uint32_t* pte_vaddr = getPtePtr(vaddr);
        uint32_t* pde_vaddr = getPdePtr(vaddr);
        if((((*pde_vaddr)&PG_P_1)== 0) || (((*pte_vaddr)&PG_P_1)==0)){
            if(mallocOnePageByVaddr(PF_USER,(void*)vaddr)==NULL)
                return -1;
        }
        vaddr += PG_SIZE;
    }
    sys_lseek(fd,prog_header->p_offset,SEEK_SET);
    sys_read(fd,(void*)prog_header->p_vaddr,prog_header->p_filesz);
    return 0;
}



static int32_t loadExecFile(const char*path)
{
    struct Elf32_Ehdr elf_header;
    struct Elf32_Phdr prog_header;
    memset(&elf_header,0,sizeof(struct Elf32_Ehdr));
    int fd = sys_open(path,O_RDONLY);
    if(fd == -1) return -1;
    int32_t elf_header_size = sys_read(fd,(char*)&elf_header,sizeof(struct Elf32_Ehdr));
    if(elf_header_size != sizeof(struct Elf32_Ehdr)){
        sys_close(fd);   
        return -1;
    }
    if(memcmp(elf_header.e_ident,"\177ELF\1\1\1",7)!=0||elf_header.e_type!=2\
     ||elf_header.e_machine!=3\
     ||elf_header.e_version!=1\
     ||elf_header.e_phrnum>1024\
     ||elf_header.e_phrentsize != sizeof(struct Elf32_Phdr)){
        sys_close(fd);
        return -1;
    }

    uint32_t prog_idx = 0;
    while(prog_idx < elf_header.e_phrnum){
        memset(&prog_header,0,sizeof(struct Elf32_Phdr));
        sys_lseek(fd,elf_header.e_phroff+prog_idx*elf_header.e_phrentsize,SEEK_SET);
        int32_t prog_size = sys_read(fd,(char*)&prog_header,sizeof(struct Elf32_Phdr));
        if(prog_size!=sizeof(struct Elf32_Phdr)){
            sys_close(fd);
            return -1;
        }
        if(prog_header.p_type == PT_LOAD){
            if(loadSegment(&prog_header,fd)==-1){
                sys_close(fd);
            }
        }
        prog_idx ++;
    }
    return elf_header.e_entry;
}


int32_t sys_execv(const char* file_path,char*argv[])
{
    uint32_t argc = 0;
    while(argv[argc]!=NULL) argc++;
    int32_t enter_point = loadExecFile(file_path);
    if(enter_point == -1){
        return -1;
    }
    struct task_struct* pcb = getpcb();
    struct intr_stack* intr_stack = (struct intr_stack*)(((uint32_t)pcb & 0xfffff000) + PG_SIZE - sizeof(struct intr_stack));
    intr_stack -> ebx = (uint32_t)argv;
    intr_stack -> ecx = argc;
    intr_stack -> eax = intr_stack->ebp = intr_stack->edi = intr_stack->edx = intr_stack->esi = 0;
    intr_stack -> eip = (void*)enter_point; 
    intr_stack -> esp = (void*)0xc0000000;
    asm volatile("movl %0,%%esp;jmp int_exit"::"g"(intr_stack):"memory");
    return 0;
}
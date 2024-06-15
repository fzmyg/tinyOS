#ifndef __PROCESS_H__
#define __PROCESS_H__

#include"thread.h"
#define KERNEL_VADDR_START 0xc0000000
#define USER_VADDR_START   0x08048000
#define USER_STACK3_VADDR_START (0xc0000000-0x1000)
#define DEFAULT_PROI 31
extern void initUserVaddrPool(struct task_struct*user_prog);

extern void* createPDT(void);

extern void activateProcess(struct task_struct*pthread);

extern void activatePDT(struct task_struct*pcb);

extern void executeProcess(void* filename,char*process_name);

extern void init(void);
#endif

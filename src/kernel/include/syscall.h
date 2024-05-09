#ifndef __SYS_CALL__
#define __SYS_CALL__
#include"thread.h"
#define SYSCALL_NR 32
//extern void* syscall_table[SYSCALL_NR];
enum SYSCALL_OFFSET{
    GET_PID = 0,
};
extern void initSyscall(void);

extern pid_t getpid(void);
#endif
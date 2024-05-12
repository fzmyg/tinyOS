#ifndef __SYS_CALL_INIT_H__
#define __SYS_CALL_INIT_H__
#define SYSCALL_NR 32
extern void* syscall_table[SYSCALL_NR];
enum SYSCALL_OFFSET{
    GET_PID = 0,
    WRITE = 1,
};
extern void initSyscall(void);

#endif
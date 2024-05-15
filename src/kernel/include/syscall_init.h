#ifndef __SYS_CALL_INIT_H__
#define __SYS_CALL_INIT_H__
#define SYSCALL_NR 32
enum SYSCALL_OFFSET{
    GET_PID = 0,
    WRITE = 1,
    MALLOC = 2,
    FREE = 3,
};
extern void initSyscall(void);

#endif
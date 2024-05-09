#include"syscall.h"
#include"string.h"
#include"thread.h"
#define _SYSCALL0(NUMBER) ({int ret_val = 0;\
                            asm volatile("int $0x80":"=a"(ret_val):"a"(NUMBER):"memory");\
                            ret_val;})
#define _SYSCALL1(NUMBER,ARG1) ({int ret_val = 0;\
                            asm volatile("int $0x80":"=a"(ret_val):"a"(NUMBER),"b"(ARG1):"memory");\
                            ret_val;})
#define _SYSCALL2(NUMBER,ARG1,ARG2) ({int ret_val = 0;\
                            asm volatile("int $0x80":"=a"(ret_val):"a"(NUMBER),"b"(ARG1),"c"(ARG2):"memory");\
                            ret_val;})
#define _SYSCALL3(NUMBER,ARG1,ARG2,ARG3) ({int ret_val = 0;\
                            asm volatile("int $0x80":"=a"(ret_val):"a"(NUMBER),"b"(ARG1),"c"(ARG2),"d"(ARG3):"memory");\
                            ret_val;})
void* syscall_table[SYSCALL_NR];

void initSyscall(void)
{
    put_str("init syscall start\n");
    syscall_table[GET_PID]=&sys_getpid;
    put_str("init syscall done\n");
}

pid_t getpid(void)
{
    return _SYSCALL0(GET_PID);
}

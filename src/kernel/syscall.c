#include"syscall.h"
#include"string.h"
#include"thread.h"
#include"syscall_init.h"
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

pid_t getpid(void)
{
    return _SYSCALL0(GET_PID);
}

uint32_t write(const char*str)
{
    return _SYSCALL1(WRITE,str);
}

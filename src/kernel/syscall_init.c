#include"syscall_init.h"
#include"thread.h"
#include"stdint.h"
#include"console.h"
#include"print.h"
#include"string.h"
#include"memory.h"
#include"fs.h"
void* syscall_table[SYSCALL_NR]={0};

static pid_t sys_getpid(void)
{
    return getpcb()->pid;
}

static uint32_t sys_print(const char*s)
{
    console_put_str(s);
    return strlen(s);
}

void initSyscall(void)
{
    put_str("init syscall start\n");
    syscall_table[GET_PID]=&sys_getpid;
    syscall_table[PRINT]=&sys_print;
    syscall_table[MALLOC]=&sys_malloc;
    syscall_table[FREE]=&sys_free;
    syscall_table[OPEN]=&sys_open;
    syscall_table[CLOSE]=&sys_close;
    syscall_table[WRITE]=&sys_write;
    put_str("init syscall done\n");
}
#include"syscall_init.h"
#include"thread.h"
#include"stdint.h"
#include"console.h"
#include"print.h"
#include"string.h"
#include"memory.h"
#include"fs.h"
#include"fork.h"
#include"exec.h"
#include"wait_exit.h"
void* syscall_table[SYSCALL_NR]={0};

static pid_t sys_getpid(void)
{
    return getpcb()->pid;
}

extern void cls(void);
static void sys_clr_screen(void)
{
    cls();
}

extern void set_cursor(int pos);
static void sys_setcursor(int pos)
{
    if(pos>=0&&pos<2000){
        set_cursor(pos);
    }
}

void initSyscall(void)
{
    put_str("init syscall start\n");
    syscall_table[GET_PID]=&sys_getpid; 
    syscall_table[MALLOC]=&sys_malloc;
    syscall_table[FREE]=&sys_free;
    syscall_table[OPEN]=&sys_open;
    syscall_table[CLOSE]=&sys_close;
    syscall_table[WRITE]=&sys_write;
    syscall_table[READ]=&sys_read;
    syscall_table[LSEEK]=&sys_lseek;
    syscall_table[UNLINK]=&sys_unlink;
    syscall_table[MKDIR]=&sys_mkdir;
    syscall_table[OPENDIR]=&sys_opendir;
    syscall_table[CLOSEDIR]=&sys_closedir;
    syscall_table[READDIR]=&sys_readdir;
    syscall_table[REWINDDIR]=&sys_rewinddir;
    syscall_table[RMDIR]=&sys_rmdir;
    syscall_table[GETCWD]=&sys_getcwd;
    syscall_table[CHDIR]=&sys_chdir;
    syscall_table[STAT] = &sys_stat;
    syscall_table[CLR_SCREEN] = &sys_clr_screen;
    syscall_table[SET_CURSOR] = &sys_setcursor;
    syscall_table[FORK]=&sys_fork;
    syscall_table[PS]=&sys_ps;
    syscall_table[EXECV]=&sys_execv;
    syscall_table[WAIT]=&sys_wait;
    syscall_table[EXIT]=&sys_exit;
    syscall_table[DUP]=&sys_dup;
    put_str("init syscall done\n");
}
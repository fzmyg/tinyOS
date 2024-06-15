#ifndef __SYS_CALL_INIT_H__
#define __SYS_CALL_INIT_H__
#define SYSCALL_NR 32
enum SYSCALL_OFFSET{
    GET_PID = 0,
    MALLOC,
    FREE,
    OPEN,
    CLOSE,
    WRITE,
    READ,
    LSEEK,
    UNLINK,
    MKDIR,
    OPENDIR,
    CLOSEDIR,
    READDIR,
    REWINDDIR,
    RMDIR,
    GETCWD,
    CHDIR,
    STAT,
    CLR_SCREEN,
    FORK,
};
extern void initSyscall(void);

#endif
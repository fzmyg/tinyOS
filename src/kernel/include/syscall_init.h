#ifndef __SYS_CALL_INIT_H__
#define __SYS_CALL_INIT_H__
#define SYSCALL_NR 32
enum SYSCALL_OFFSET{
    GET_PID = 0,
    PRINT = 1,
    MALLOC = 2,
    FREE = 3,
    OPEN = 4,
    CLOSE = 5,
    WRITE = 6,
    READ = 7,
    LSEEK = 8,
    UNLINK = 9,
    MKDIR = 10,
    OPENDIR = 11,
    CLOSEDIR = 12,
    READDIR = 13,
    REWINDDIR = 14,
    RMDIR = 15,
    GETCWD = 16,
    CHDIR = 17,
    STAT = 18,
};
extern void initSyscall(void);

#endif
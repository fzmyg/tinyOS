#ifndef __SYS_CALL__
#define __SYS_CALL__
#include"stdint.h"
#include"thread.h"
/*获取进程ID*/
extern pid_t getpid(void);

/*项屏幕输出*/
extern uint32_t print(const char*s);

/*申请内存*/
extern void* malloc(uint32_t size);

/*回收内存*/
extern void free(void*ptr);

extern int open(const char* path,uint32_t o_mode);

extern void close(int fd);

extern int write(int fd,const char* buf,uint32_t count);

#endif
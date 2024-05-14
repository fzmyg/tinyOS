#ifndef __SYS_CALL__
#define __SYS_CALL__
#include"stdint.h"
#include"thread.h"
/*获取进程ID*/
extern pid_t getpid(void);

/*项屏幕输出*/
extern uint32_t write(const char*s);

/*申请内存*/
extern void* malloc(uint32_t size);

#endif
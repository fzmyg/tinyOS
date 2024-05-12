#ifndef __SYS_CALL__
#define __SYS_CALL__
#include"stdint.h"
#include"thread.h"
extern pid_t getpid(void);

extern uint32_t write(const char*s);

#endif
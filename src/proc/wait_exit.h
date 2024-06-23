#ifndef __WAIT_EXIT__
#define __WAIT_EXIT__
#include"thread.h"
#include"stdint.h"
extern pid_t sys_wait(int32_t*status);

extern int32_t sys_exit(int32_t status);
#endif
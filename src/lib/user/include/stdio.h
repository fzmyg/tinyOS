#ifndef __STDIO_H__
#define __STDIO_H__
#include"stdint.h"

extern uint32_t printf(const char*fotmat,...);

extern uint32_t sprintf(char*buf,const char*format,...);
#endif
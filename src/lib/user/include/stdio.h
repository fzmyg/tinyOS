#ifndef __STDIO_H__
#define __STDIO_H__
#include"stdint.h"

//函数可变参数宏
typedef char* va_list;
//函数可变参数宏
#define va_start(parg,argv) (parg = (va_list)&argv)                         //初始化参数指针
#define va_arg(parg,type) (*((type*)((parg+=sizeof(type))-sizeof(type))))   //移动参数指针并返回函数参数
#define va_end(parg) parg = NULL                                            //将参数指针置空

extern uint32_t printf(const char*fotmat,...);

extern uint32_t vsprintf(char*buf,const char*format,va_list va);

extern uint32_t sprintf(char*buf,const char*format,...);
#endif
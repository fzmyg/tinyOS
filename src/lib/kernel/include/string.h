#ifndef __STRING_H__
#define __STRING_H__
#include"stdint.h"
#define NULL 0
extern void memset(void*dest,uint8_t val,uint32_t size);
extern void* memcpy(void*dst,const void*src,uint32_t size);
extern int8_t memcmp(const void* a,const void*b,uint32_t size);

extern char* strcpy(char*dst,const char*src);
extern char*strncpy(char*dst,const char*src,uint32_t size);
extern uint32_t strlen(const char*s);
extern int8_t strcmp(const char*a,const char*b); //ok
extern int8_t strncmp(const char*a,const char*b,uint32_t size);
extern uint32_t strchrs(const char*str,const uint8_t ch);
extern char* strchr(const char*str,const uint8_t ch);
extern char* strrchr(const char*str,const char ch);
extern char* strcat(char*dst_,const char*src_);
extern char* strncat(char*dst_,const char*src_,uint32_t size);
#endif

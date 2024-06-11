#include"string.h"
#include"debug.h"
#include"print.h"
#include"stdint.h"
/*
extern void memset(void*dest,uint8_t val,uint32_t size);
extern void* memcpy(void*dst,const void*src,uint32_t size);
extern int8_t memcmp(const void* a,const void*b,uint32_t size);

extern char* strcpy(char*dst,const char*src);
extern char* strncpy(char*dst,const char*src,uint32_t size);
extern uint32_t strlen(const char*s);
extern int8_t strcmp(const char*a,const char*b);
extern int8_t strncmp(const char*a,const char*b,uint32_t size);
extern char* strchr(const char*str,const uint8_t ch);
extern uint32_t strchrs(const char*str,const uint8_t ch);
extern char* strcat(char*dst_,const char*src_);
extern char* strncat(char*dst_,const char*src_,uint32_t size);
*/

void memset(void*dst,uint8_t val,uint32_t size)
{
	ASSERT(dst!=NULL);
	uint8_t* p = (uint8_t*)dst;
	while(size-- > 0)
		*p++=val;
}

void* memcpy(void*dst_,const void*src_,uint32_t size)
{
	ASSERT(dst_!=NULL&&src_!=NULL);
	uint8_t*dst = (uint8_t*)dst_;
	const uint8_t*src = (const uint8_t*)src_;
	while(size-- > 0){
		*dst++=*src++;
	}
	return dst_;
}

int8_t memcmp(const void*a_,const void*b_,uint32_t size)
{
	ASSERT(a_!=NULL&&b_!=NULL);
	const uint8_t* a = (const uint8_t*)a_;
	const uint8_t* b = (const uint8_t*)b_;
	while(size-- > 0){
		if(*a != *b){
			return *a>*b?1:-1;
		}
		a++;b++;
	}
	return 0;
}

char* strncpy(char*dst,const char*src,uint32_t size)
{
	ASSERT(dst!=NULL&&src!=NULL);
	while(size-- > 0){
		*dst++ = *src++; 	
	}
	return dst;
}

char* strcpy(char*dst,const char*src)
{
	ASSERT(dst!=NULL&&src!=NULL);
	while((*dst=*src)!=0){
		dst++;src++;
	}
	return dst;
}

int8_t strcmp(const char*a,const char*b)
{
	ASSERT(a!=NULL&&b!=NULL);
	while(*a!=0&&*b!=0&&*a==*b){
		a++;b++;
	}
	if(*a==0&&*b!=0)
		return -1;
	else if(*a!=0&&*b==0)
		return 1;
	else if(*a==0&&*b==0)
		return 0;
	else
		return *a>*b?1:-1;
}
int8_t strncmp(const char* a ,const char*b,uint32_t size)
{
	ASSERT(a!=NULL&&b!=NULL);
	while(*a!=0&&*b!=0&&*a==*b&&size>0){
		a++;b++;size--;
	}
	if(*a==0&&*b!=0)
		return -1;
	else if(*a!=0&&*b==0)
		return 1;
	else if(*a==0&&*b==0)
		return 0;
	else
		return *(a-1)<*(b-1)?-1:*(a-1)>*(b-1);
}

uint32_t strlen(const char*s)
{
	ASSERT(s!=NULL);
	const char*p=s;
	while(*p!=0){
		p++;
	}
	return p-s;
}

uint32_t strchrs(const char*str,uint8_t ch)
{
	ASSERT(str!=NULL);
	const char*p=str;
	int count = 0;
	while(*p!=0){
		if(*p==ch) count++;
		p++;
	}
	return count;
}
char* strchr(const char*str,const uint8_t ch)
{
	ASSERT(str!=NULL)
	while(*str!=ch)str++;
	return (char*)str;
}

char* strrchr(const char*str,const char ch)
{
	char* p = (char*)str+strlen(str)-1;
	while((uint32_t)p>=(uint32_t)str&&*p!=ch){
		p--;
	}
	if(p<str) return NULL;
	return p;
}

char * strcat(char*dst_,const char*src_)
{
	ASSERT(dst_!=NULL&&src_!=NULL);
	char * p = dst_;
	while(*p)p++;
	while(*src_){
		*p = *src_;
		p++;src_++;
	}
	*p=*src_;
	return dst_;
}


char * strncat(char*dst_,const char*src_,uint32_t size)
{
	ASSERT(dst_!=NULL&&src_!=NULL);
	char * p = dst_;
	while(*p)p++;
	while(*src_&&size>0){
		*p = *src_;
		p++;src_++;size--;
	}
	*p=0;
	return dst_;
}

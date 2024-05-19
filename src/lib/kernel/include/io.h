#ifndef __LIB_IO_H__
#define __LIB_IO_H__
#include"stdint.h"
/*output 1B to port*/
static inline void outb(uint16_t port,uint8_t data)
{
	asm volatile ("out %b0,%w1"::"a"(data),"d"(port));
}
/*ouput serious of data to a port*/
static inline void outsb(uint16_t port,const void*addr,uint32_t word_cnt)
{
	asm volatile ("cld; rep outsw":"+S"(addr),"+c"(word_cnt):"d"(port));
}
/*read 1B from port*/
static inline uint8_t inb(uint16_t port)
{	
	uint8_t data;
	asm volatile ("in %w1,%b0":"=a"(data):"d"(port));
	return data;
}
/*read  serious of data from port*/
static inline void insw(uint16_t port,void *addr,uint32_t word_cnt)
{
	asm volatile ("movw $0x0010,%%ax;"\
		       "movw %%ax,%%es;"\
		       "cld;"\
		       "rep insw;"\
				:"+D"(addr),"+c"(word_cnt):"d"(port):"memory");
}
#endif

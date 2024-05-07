#ifndef __MEMORY_H__
#define __MEMORY_H__
#include"bitmap.h"
#include"stdint.h"
#define KERNEL_BITMAP_START 0xc009a000
#define KERNEL_HEAP_START   0xc0100000
#define PG_SIZE 0x1000
#define PG_US_S 0b000
#define PG_US_U 0b100
#define PG_RW_R 0b000
#define PG_RW_W 0b010
#define PG_P_0  0b000
#define PG_P_1  0b001
enum pool_flags{
	PF_KERNEL,
	PF_USER
};


typedef struct vmpool{
	Bitmap bitmap;
	uint32_t vm_start;
}vmpool;


extern void initMemPool(void);

extern uint32_t * getPdePtr(uint32_t vaddr);

extern uint32_t * getPtePtr(uint32_t vaddr);

extern void* mallocPage(enum pool_flags pf,uint32_t pg_cnt);

extern void* mallocKernelPage(uint32_t pg_cnt);

extern void* mallocUserPage(uint32_t pg_cnt);

extern void freeKernelPage(void*vaddr);

extern void freePcb(void*vaddr);

extern void* addr_v2p(void* vaddr);

extern void* mallocOnePageByVaddr(enum pool_flags pf,void* vaddr);

#endif

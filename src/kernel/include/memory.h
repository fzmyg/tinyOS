#ifndef __MEMORY_H__
#define __MEMORY_H__
#include"bitmap.h"
#include"stdint.h"
#include"list.h"
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

/*虚拟内存池*/
typedef struct vmpool{
	Bitmap bitmap;     /*内存池位图*/
	uint32_t vm_start; /*虚拟内存起始内存*/
}vmpool;

/*内存块*/
typedef struct mem_block{
	struct list_elem free_elem;
}mem_block;

/*内存块描述符*/
typedef struct mem_block_desc{
	struct list free_list;   /*可用的mem_block组成的链表*/
	uint32_t block_size;
	uint32_t blocks_per_arena;
}mem_block_desc;

/* 申请内存4KB起始数据结构*/
typedef struct arena{
	struct mem_block_desc* desc; //指向pcb中desc
	uint32_t cnt; //large 为0，为内存块数量，large为1，为页数量
	bool large;  //为true表示分配arena大小为4KB*n
}arena;

#define MEM_DESC_CNT 7 		/*16 32 64 128 256 512 1024*/

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

extern void initMemBlockDesc(struct mem_block_desc * k_mem_block_descs);

extern void*sys_malloc(uint32_t cnt);
#endif

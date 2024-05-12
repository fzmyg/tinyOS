/*
 * 	typedef struct vmpool{
 *         	Bitmap bitmap;
 *              uint32_t vm_start;
 *      }vmpool;
 *
 *      typedef struct pool{
 *              Bitmap bitmap;
 *              uint32_t m_start;
 *              uint32_t m_length;
 *      }pool;
 * */
#include"memory.h"
#include"debug.h"
#include"string.h"
#include"print.h"
#include"interrupt.h"
#include"thread.h"
#include"sync.h"

typedef struct pool{
        struct lock lock;   //内存池锁
        Bitmap bitmap;		//内存位图
        uint32_t m_start;   //内存起始位置
        uint32_t m_length;  //内存池管理长度
}pool;

#define PDE_INDEX(ADDR) ((ADDR & 0xffc00000)>>22)  //获取pde索引
#define PTE_INDEX(ADDR) ((ADDR & 0x003ff000)>>12)  //获取pte索引

struct pool kernel_pool,user_pool; //定义内核物理池，用户物理池

struct vmpool kernel_vmpool;		//内核虚拟内存池

//初始化内核和用户物理内存池，内核虚拟内存池
static void initAllPool(uint32_t all_mem)
{
	uint32_t used_mem = 0x100000 + PG_SIZE * 256 ; //1 pdt + 1 + 254
	uint32_t free_mem = all_mem - used_mem;
	ASSERT(free_mem > 0);
	uint32_t kernel_free_mem = free_mem / 2;
	uint32_t user_free_mem = free_mem - kernel_free_mem;
	uint32_t kernel_free_page = kernel_free_mem  / PG_SIZE;
	uint32_t user_free_page = user_free_mem / PG_SIZE;
	uint32_t kernel_bitmap_byte_len = kernel_free_page / 8;	
	uint32_t user_bitmap_byte_len = user_free_page / 8;	
	kernel_pool.bitmap.pbitmap = (void*)KERNEL_BITMAP_START;	
	user_pool.bitmap.pbitmap =(void*)( KERNEL_BITMAP_START + kernel_bitmap_byte_len);
	kernel_pool.bitmap.bitmap_byte_len = kernel_bitmap_byte_len;
	user_pool.bitmap.bitmap_byte_len = user_bitmap_byte_len;
	initBitmap(&kernel_pool.bitmap);
	initBitmap(&user_pool.bitmap);
	initLock(&kernel_pool.lock);	
	initLock(&user_pool.lock);
	kernel_pool.m_start = used_mem;
	kernel_pool.m_length = kernel_free_mem;
	user_pool.m_start = kernel_pool.m_start + kernel_pool.m_length;
	user_pool.m_length = user_free_mem;
	
	kernel_vmpool.bitmap.bitmap_byte_len = kernel_bitmap_byte_len;
	kernel_vmpool.bitmap.pbitmap = (void*)(KERNEL_BITMAP_START + kernel_bitmap_byte_len + user_bitmap_byte_len);
	kernel_vmpool.vm_start = KERNEL_HEAP_START;
	initBitmap(&kernel_vmpool.bitmap);  

	put_str("[kernel bitmap start         ]:");put_int(KERNEL_BITMAP_START);put_char(10);
	put_str("[kernel bitmao end           ]:");put_int(KERNEL_BITMAP_START+kernel_pool.bitmap.bitmap_byte_len-1);put_char(10);
	put_str("[user bitmap start           ]:");put_int(KERNEL_BITMAP_START+kernel_pool.bitmap.bitmap_byte_len);put_char(10);
	put_str("[user bitmap end             ]:");put_int(KERNEL_BITMAP_START+kernel_pool.bitmap.bitmap_byte_len+user_pool.bitmap.bitmap_byte_len-1);put_char(10);
	put_str("[kernel virtual  bitmap start]:");put_int(KERNEL_BITMAP_START+kernel_pool.bitmap.bitmap_byte_len+user_pool.bitmap.bitmap_byte_len);put_char(10);
	put_str("[kernel virtual  bitmao end  ]:");put_int(KERNEL_BITMAP_START+kernel_pool.bitmap.bitmap_byte_len+user_pool.bitmap.bitmap_byte_len+kernel_vmpool.bitmap.bitmap_byte_len-1);put_char(10);
	
	put_str("[kernel physical memory start]:");put_int(kernel_pool.m_start);put_char(10);
	put_str("[kernel physical memory end  ]:");put_int(kernel_pool.m_start+kernel_pool.m_length-1);put_char(10);
	put_str("[user physical memory start  ]:");put_int(user_pool.m_start);put_char(10);
	put_str("[user physical memory end    ]:");put_int(user_pool.m_start+user_pool.m_length-1);put_char(10);
	put_str("[kernel virtual memory start ]:");put_int(kernel_vmpool.vm_start);put_char(10);
}

//对外初始化接口
void initMemPool(void)
{
        put_str("initing all memory pools\n");
        uint32_t all_mem = (*(uint32_t*)(0xb03));
        initAllPool(all_mem);
        put_str("init all memory pools done\n");
}

//获取pde地址
uint32_t * getPdePtr(uint32_t vaddr)
{
	return (uint32_t*)(0xfffff000+PDE_INDEX(vaddr)*4);
}

//获取pte地址
uint32_t * getPtePtr(uint32_t vaddr)
{
	return (uint32_t*)(0xffc00000 + (PDE_INDEX(vaddr)<<12) + PTE_INDEX(vaddr)*4);
}

/* 从物理内存池中开辟内存，pf选择从用户还是内核物理池中开辟
 * 成功返回物理地址，失败返回空*/
static void* palloc(enum pool_flags pf,uint32_t pg_cnt)
{
	struct pool * pp = (pf==PF_KERNEL?&kernel_pool:&user_pool);
	acquireLock(&pp->lock);
	int bit_index = (int)scanBitmap(&pp->bitmap,pg_cnt);
	if(bit_index  == -1)
		return NULL;
	uint32_t cnt = pg_cnt;
        while(cnt >0) {setBitmap(&pp->bitmap,bit_index+cnt-1,1);cnt--;}
	releaseLock(&pp->lock);
	return (void*)(pp->m_start+bit_index*PG_SIZE);
}

/*从虚拟内存池中开辟虚拟内存
 *成功返虚拟地址，失败返回空*/
static void* valloc(struct vmpool *pp,uint32_t pg_cnt)
{
        int bit_index = (int)scanBitmap(&pp->bitmap,pg_cnt);
        if(bit_index  == -1)
                return NULL;
        uint32_t cnt = pg_cnt;
        while(cnt >0) {setBitmap(&pp->bitmap,bit_index+cnt-1,1);cnt--;}
        return (void*)(pp->vm_start+bit_index*PG_SIZE);
}

/* 映射虚拟地址到物理地址 （单位：页）*/
static void mapVaddr2Paddr(void*_vaddr,void* _paddr)
{
	uint32_t vaddr = (uint32_t)_vaddr , paddr = (uint32_t)_paddr;
	uint32_t * pde_ptr = getPdePtr((uint32_t)vaddr);
	uint32_t * pte_ptr = getPtePtr((uint32_t)vaddr);
	if((*pde_ptr & PG_P_1) == 0){ /*页表不存在*/
		void* page_table_paddr = palloc(PF_KERNEL,1);/*从内核物理池给页表申请物理地址*/
		*pde_ptr = ((uint32_t)page_table_paddr | PG_US_U | PG_RW_W | PG_P_1);/*设置页目录项为页表起始物理地址*/ 
		memset((void*)((uint32_t)pte_ptr&0xfffff000/*pte_ptr不一定是页表首项,因此需&0xfffff000*/),0,PG_SIZE);//初始化新申请的页表
		*pte_ptr = ((paddr & 0xfffff000) | PG_US_U | PG_RW_W | PG_P_1);/*将页表项设为要映射的物理地址*/
	}else{
		ASSERT((*pte_ptr & PG_P_1) == 0);
		*pte_ptr = ((paddr & 0xfffff000) | PG_US_U | PG_RW_W | PG_P_1);/*页表存在则直接映射*/
	}

}

/*
 * 申请内存  若pf==PF_KERNEL则在内核虚拟池申请虚拟地址并在内核物理池申请物理地址
 *          若pf==PF_USER 则在当前PCB虚拟池申请虚拟地址并在用户物理池申请物理地址 
 */
void* mallocPage(enum pool_flags pf,uint32_t pg_cnt)
{
	void*vmaddr_start = NULL;
	if(pf==PF_KERNEL){
		vmaddr_start = valloc(&kernel_vmpool,pg_cnt);  //pf==PF_KERNEL则在内核虚拟池申请虚拟地址
	}else{
		struct task_struct* pcb = getpcb();
		vmaddr_start = valloc(&pcb->vaddr_pool,pg_cnt);//pf==PF_USER 则在当前PCB虚拟池申请虚拟地址 
	}
	if(vmaddr_start == NULL) return NULL;	
	uint32_t count = 0;
	while(count++ < pg_cnt){
		void* phyaddr_start=palloc(pf,1);
		if(phyaddr_start == NULL) return NULL;
		mapVaddr2Paddr((void*)((uint32_t)vmaddr_start+(count-1)*PG_SIZE),(void*)phyaddr_start);
	}	
	return vmaddr_start;
}
/*
 *  为内核进程申请内核物理空间
 */
void* mallocKernelPage (uint32_t pg_cnt)
{
	void*vmaddr_start = mallocPage(PF_KERNEL,pg_cnt);
	if(vmaddr_start !=  NULL) memset(vmaddr_start,0,PG_SIZE*pg_cnt);
	return vmaddr_start;
}
/*
 *  为用户进程申请用户物理空间
 */
void* mallocUserPage(uint32_t pg_cnt)
{
	void*vmaddr_start = mallocPage(PF_USER,pg_cnt);
	if(vmaddr_start !=  NULL) memset(vmaddr_start,0,PG_SIZE*pg_cnt);
	return vmaddr_start;
}
/*
 *  指定虚拟地址申请一页内存
 */
void* mallocOnePageByVaddr(enum pool_flags pf,void* vaddr)
{
	uint32_t v_addr = (uint32_t )vaddr;
	struct task_struct*pcb  = getpcb();
	struct vmpool* pvp = (pf==PF_KERNEL&&pcb->pgdir_vaddr==NULL)?&kernel_vmpool:&pcb->vaddr_pool;
	uint32_t bit_index = (v_addr - pvp->vm_start)/PG_SIZE;
	if(bitIsUsed(&pvp->bitmap,bit_index)){ 
		return NULL;
	}
	setBitmap(&pvp->bitmap,bit_index,1); 
	void*paddr=palloc(pf,1);
	mapVaddr2Paddr(vaddr,paddr);
	return vaddr;
}

/* 释放内核页 */
void freeKernelPage(void*vaddr)
{
	uint32_t*ppte = getPtePtr((uint32_t)vaddr);
	int bit_index = (((uint32_t)vaddr&0xfffff000)  - kernel_vmpool.vm_start)/0x1000;
	setBitmap(&(kernel_vmpool.bitmap),bit_index,0);
	uint32_t paddr = (*ppte)&0xfffff000;
	bit_index = (paddr - kernel_pool.m_start)/0x1000;
        setBitmap(&(kernel_vmpool.bitmap),bit_index,0);
	(*ppte) = 0;
}

extern void switch_to_and_free_end(void);
void freePcb(void*vaddr)
{
	uint32_t*ppte = getPtePtr((uint32_t)vaddr);
	int bit_index = (((uint32_t)vaddr&0xfffff000)  - kernel_vmpool.vm_start)/0x1000;
	setBitmap(&(kernel_vmpool.bitmap),bit_index,0);
	uint32_t paddr = (*ppte)&0xfffff000;
	bit_index = (paddr - kernel_pool.m_start)/0x1000;
	setBitmap(&(kernel_vmpool.bitmap),bit_index,0);
	(*ppte) = 0;
	asm volatile("jmp switch_to_and_free_end");
}

/* 将虚拟地址转换为物理地址 */
void* addr_v2p(void* vaddr)
{
	uint32_t * ppte = getPtePtr((uint32_t)vaddr);
	return (void*)(((*ppte) & (uint32_t)0xfffff000)|((uint32_t)(vaddr)&(uint32_t)0x00000fff));
}


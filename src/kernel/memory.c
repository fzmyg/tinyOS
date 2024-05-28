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
#define DIV_ROUND_UP(a,b) ((a+b-1)/b)
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

struct mem_block_desc k_mem_block_descs[MEM_DESC_CNT]; /*内核内存块描述符数组*/

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
		initMemBlockDesc(k_mem_block_descs);
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
	memset(vmaddr_start,0,pg_cnt*PG_SIZE);
	return vmaddr_start;
}
/*
 *  为内核进程申请内核物理空间
 */
void* mallocKernelPage (uint32_t pg_cnt)
{
	void*vmaddr_start = mallocPage(PF_KERNEL,pg_cnt);
	//if(vmaddr_start !=  NULL) memset(vmaddr_start,0,PG_SIZE*pg_cnt);
	return vmaddr_start;
}
/*
 *  为用户进程申请用户物理空间
 */
void* mallocUserPage(uint32_t pg_cnt)
{
	void*vmaddr_start = mallocPage(PF_USER,pg_cnt);
	//if(vmaddr_start !=  NULL) memset(vmaddr_start,0,PG_SIZE*pg_cnt);
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


/*初始化内存管理符*/
void initMemBlockDesc(struct mem_block_desc*descs)
{
	//16 32 64 128 256 512 1024
	uint32_t size = 16;
	uint32_t desc_index = 0;
	for(;desc_index<MEM_DESC_CNT;desc_index++){
		list_init(&descs[desc_index].free_list); 
		descs[desc_index].block_size=size;
		descs[desc_index].blocks_per_arena = (PG_SIZE-sizeof(struct arena))/size;
		size*=2;
	}
}

/* 在arena中凭借index索引找到mem_block地址*/
static struct mem_block* arena2block(struct arena*arena,uint32_t index)
{
	return (struct mem_block*)((uint32_t)arena + sizeof(struct arena) + index*(arena->desc->block_size));
}

/* 凭借mem_block找到arena */
static struct arena* block2arena(struct mem_block* block)
{
	return (struct arena*)((uint32_t)block & 0xfffff000);
}

/*内核态申请内存 
 *1.根据调用者选择内存池和pf位 ----> 2.通过mallocPage(pf,size)申请内存 ----> 判断为大块内存 ----> 直接返回内存
 * 																   |----> 判断为小块内存 ----> 寻找相应内存块描述符 ----> 若内存块描述符容量为空 ----> 通过arena->desc->free_list 串连起内存块(mem_block) ----> 返回首个mem_block
 *																												|----> 若内存块描述符容量不空 ----> 返回首个mem_block
 */
void * sys_malloc(uint32_t size)
{
	enum pool_flags pf=PF_USER;
	struct pool* mem_pool = NULL; 			
	uint32_t pool_size = 0; 	  			
	struct mem_block_desc*descs = NULL; 	
	struct task_struct*  cur_pcb = getpcb();
	/*根据调用者初始化参数*/
	if(cur_pcb->pgdir_vaddr == NULL){ 		
		pf = PF_KERNEL;
		pool_size = kernel_pool.m_length;
		mem_pool = & kernel_pool;
		descs = k_mem_block_descs;
	}else{	
		pf = PF_USER;
		pool_size = user_pool.m_length;
		mem_pool= &user_pool;
		descs = cur_pcb->descs;
	}

	if((size>0&&size<pool_size)== false){ //申请内存量超出内存池大小
		return NULL;                      //直接返回空
	}
	struct arena* arena=NULL;		  //申请内存起始地址
	//acquireLock(&mem_pool->lock); 	  //修改物理池需主动获取锁
	if(size>1024){ /*申请内存数大于1024B*/
		uint32_t page_cnt = DIV_ROUND_UP(size+sizeof(struct arena),PG_SIZE); //申请PG_SIZE整数被内存
		arena = mallocPage(pf,page_cnt);
		if(arena == NULL){	
			//releaseLock(&mem_pool->lock);
			return NULL; /*申请失败 则返回NULL*/
		}
		memset(arena,0,page_cnt*PG_SIZE);
		arena -> desc = NULL;
		arena -> cnt = page_cnt;
		arena -> large = true;
		//releaseLock(&mem_pool->lock);
		return (void*)(arena+1);
	}else{  /*申请数量小于1024B*/
		uint8_t desc_index; /*描述符索引*/
		while(size > descs[desc_index].block_size) desc_index++;
		if(list_empty(&descs[desc_index].free_list)){ /*内存块描述符空闲内存块队列不为空*/
			arena = mallocPage(pf,1); 				  //申请新4KB内存
			if(arena == NULL) {       				  //内存不足 直接返回
				//releaseLock(&mem_pool->lock);
				return NULL;
			}
			memset(arena,0,PG_SIZE);
			arena -> large = false;
			arena -> desc = &descs[desc_index];
			arena -> cnt = descs[desc_index].blocks_per_arena;

			uint32_t block_index  = 0;
			//操作list需关中断
			enum int_status stat = closeInt();
			struct mem_block* block=NULL; //负责遍历arene，用来组织链表
			/* 设置每个area中的mem_block */
			for(block_index = 0 ; block_index < descs[desc_index].blocks_per_arena;block_index++){
				block = arena2block(arena,block_index);
				ASSERT(find_elem(&arena->desc->free_list,&block->free_elem)==false);
				list_append(&(arena->desc->free_list),&(block->free_elem)); /*添加内存块到内存空闲队列中*/
			} 
			setIntStatus(stat);
		}
		struct mem_block* block = NULL;
		struct list_elem* pnode = list_pop(&(descs[desc_index].free_list)); 
		block = (struct mem_block*)((uint32_t)pnode-offset(struct mem_block,free_elem));
		arena=block2arena(block);
		arena -> cnt --;
		memset(block,0,descs[desc_index].block_size);
		//releaseLock(&mem_pool->lock);
		return (void*)(block);
	}
}

/*将物理内存池相应位置0*/
static void pfree(uint32_t pg_phy_addr)
{
	struct pool* mem_pool = (pg_phy_addr>=user_pool.m_start)?&user_pool:&kernel_pool;/*靠pg_phy_addr判断操作内存池对象是用户物理内存池还是内核物理内存池*/
	uint32_t bit_index = (pg_phy_addr-mem_pool->m_start)/PG_SIZE;
	acquireLock(&mem_pool->lock);
	setBitmap(&mem_pool->bitmap,bit_index,0);
	releaseLock(&mem_pool->lock);
}

/*修改页表删除虚拟内存到物理内存映射关系*/
static void unMapVaddr(uint32_t vaddr)
{
	uint32_t* pte_ptr = getPtePtr(vaddr);
	(*pte_ptr) &= (~PG_P_1); 			/*pte P位置0表示相应页不存在*/
	asm volatile ("invlpg %0"::"m"(vaddr):"memory"); /*更新页表缓存*/
}

/*将虚拟地址池置相应位0*/
static void vfree(enum pool_flags pf,void*_vaddr,uint32_t pg_cnt)
{
	struct task_struct* pcb =getpcb();
	struct vmpool * vmem_pool = (pf == PF_KERNEL)?&kernel_vmpool:&pcb->vaddr_pool; /*靠pf确定虚拟内存池操作对象是内核虚拟池还是用户PCB中虚拟池*/ 
	uint32_t bit_index = ((uint32_t)_vaddr-vmem_pool->vm_start)/PG_SIZE;
	uint32_t i = 0 ;
	for(i=0;i<pg_cnt;i++){
		setBitmap(&vmem_pool->bitmap,bit_index+i,0);
	}
}

/*内存释放总函数 1.物理位图置0，2.删除页映射，3.虚拟位图置0*/
static void mfree(enum pool_flags pf,void*_vaddr,uint32_t pg_cnt)
{
	uint32_t vaddr = (uint32_t)_vaddr,i=0;

	for(i=0;i<pg_cnt;i++){
		uint32_t paddr = (uint32_t)addr_v2p((void*)vaddr);
		pfree(paddr);         /*从物理内存池置0*/
		unMapVaddr(vaddr);    /* 页表项P位置0*/
		vaddr+= PG_SIZE;
	}
	vfree(pf,_vaddr,pg_cnt);  /*虚拟内存池置0*/
}

void sys_free(void*vaddr)
{
	struct task_struct* pcb = getpcb();
	enum pool_flags pf = pcb->pgdir_vaddr==NULL?PF_KERNEL:PF_USER; /*获取当前用户身份*/
	struct mem_block* mem_block = (struct mem_block*)vaddr;
	struct arena* arena =  block2arena(mem_block);
	ASSERT(arena->large == 0 || arena->large == 1);
	if(arena->large == true){ /*若为大块内存申请释放*/
		mfree(pf,arena,arena->cnt); /*直接释放*/
	}else{ /*小块内存申请释放*/
		enum int_status stat = closeInt();
		ASSERT(find_elem(&arena->desc->free_list,(struct list_elem*)mem_block)==false);
		list_append(&arena->desc->free_list,(struct list_elem*)mem_block); /*将mem_block 重新加入到 内存描述符的空闲队列中*/
		setIntStatus(stat);
		arena->cnt++ ;
		if(arena->cnt == arena->desc->blocks_per_arena){ /*若arena中所有内存块都在等待队列中（整页空闲），则从等待队列中删除所有mem_block节点，并清楚页表项*/
			uint32_t i = 0;
			for(i=0;i<arena->cnt;i++){ /*遍历删除所有内存节点*/
				struct mem_block* block = arena2block(arena,i); 
				stat = closeInt();
				ASSERT(find_elem(&arena->desc->free_list,(struct list_elem*)block)==true);
				list_remove((struct list_elem*)block); /*删除块节点*/
				setIntStatus(stat);
			}
			mfree(pf,vaddr,1); /*清楚页表项，恢复内存位图*/
		}
	}
}
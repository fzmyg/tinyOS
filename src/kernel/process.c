#include"process.h"
#include"global.h"
#include"stdint.h"
#include"thread.h"
#include"memory.h"
#include"string.h"
#include"interrupt.h"
#include"tss.h"
/*
 *Description: init user's virtual address pool
 *Return Value: empty
 * */
void initUserVaddrPool(struct task_struct*pthread)
{
	pthread->vaddr_pool.vm_start = USER_VADDR_START;
	uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000-USER_VADDR_START)/PG_SIZE/8,PG_SIZE);
	void*pbitmap = mallocKernelPage(bitmap_pg_cnt);
	pthread->vaddr_pool.bitmap.pbitmap = pbitmap;
	pthread->vaddr_pool.bitmap.bitmap_byte_len = (0xc0000000-USER_VADDR_START)/PG_SIZE/8;
	initBitmap(&pthread->vaddr_pool.bitmap);
}

/*
 *Description:create PDT in kernel memory pool
 *Return Value: on successful kernel's virtual address will be returned,else return NULL 
 * */
void* createPDT()
{
	void*page_dir_vaddr = mallocKernelPage(1);
	if(page_dir_vaddr == NULL) 
		return NULL;
	memcpy((void*)((uint32_t)page_dir_vaddr+768*4),(void*)(0xfffff000+768*4),255*4);
	void*paddr= addr_v2p(page_dir_vaddr);	
	((uint32_t*)page_dir_vaddr)[1023] = (((uint32_t)paddr) | PG_US_U | PG_RW_W | PG_P_1);
	return page_dir_vaddr;
}

extern void int_exit(void);
static void* processBooter(void* filename)
{
	void * function = filename;
	struct task_struct* pcb = getpcb();
	pcb->self_kernel_stack += sizeof(struct thread_stack);
	struct intr_stack* proc_stack = (struct intr_stack*)(pcb->self_kernel_stack);
	memset(proc_stack,0,sizeof(struct intr_stack));
	proc_stack->esp_dummy = 0;
	proc_stack->ds = proc_stack->fs = proc_stack->es = SELECTOR_U_DATA;
	proc_stack->eip = function;
	proc_stack->cs = SELECTOR_U_CODE;
	proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
	proc_stack->esp = (void*)((uint32_t)mallocOnePageByVaddr(PF_USER,(void*)USER_STACK3_VADDR_START)+PG_SIZE); 
	proc_stack->ss = SELECTOR_U_DATA;	
	struct intr_stack* user_stack = (struct intr_stack*)((uint32_t)proc_stack->esp-sizeof(struct intr_stack));
	memcpy(user_stack,proc_stack,sizeof(struct intr_stack));
	asm volatile("movl %0,%%esp;jmp int_exit"::"g"(proc_stack):"memory");
	return NULL;
}

static void activatePDT(struct task_struct*pcb)
{
        void* pdt_paddr = (void*)0x100000;
        uint32_t pdt_vaddr = pcb->pgdir_vaddr;
        if(pdt_vaddr!=NULL)
                pdt_paddr = addr_v2p((void*)pdt_vaddr);
        asm volatile("movl %0,%%cr3"::"r"(pdt_paddr):"memory");
}

void activateProcess(struct task_struct*pthread)
{
        activatePDT(pthread); 			//switch page table , can still visit kernel space 
        if(pthread->pgdir_vaddr!=NULL)
                update_tss_esp(pthread);
}

void executeProcess(void* filename,char*process_name)
{
	enum int_status stat = closeInt();
	struct task_struct* proc_pcb = thread_start(process_name,DEFAULT_PROI,&processBooter,filename);
	initUserVaddrPool(proc_pcb);
	proc_pcb->pgdir_vaddr = (uint32_t)createPDT();	
	setIntStatus(stat);
}


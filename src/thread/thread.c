#include"thread.h"
#include"memory.h"
#include"string.h"
#include"debug.h"
#include"interrupt.h"
#include"list.h"
#include"print.h"
#include"process.h"
#include"sync.h"
#include"global.h"
#include"process.h"
#include"fs.h"
#include"stdio.h"
#include"mount.h"
struct task_struct* main_thread;
struct list thread_ready_list;
struct list thread_all_list;
static struct list_elem* thread_tag;
struct task_struct * idle_thread;

#define UNUSED __attribute__((unused))

#define MAX_PROCESS_CNT 4096

struct pid_pool{
	struct lock lock;
	struct Bitmap bitmap;
};

struct pid_pool pid_pool;

static void initPidPool(void){
	pid_pool.bitmap.pbitmap = sys_malloc(DIV_ROUND_UP(MAX_PROCESS_CNT,8));
	ASSERT(pid_pool.bitmap.pbitmap!=NULL);
	pid_pool.bitmap.bitmap_byte_len = DIV_ROUND_UP(MAX_PROCESS_CNT,8);
	memset(pid_pool.bitmap.pbitmap,0,pid_pool.bitmap.bitmap_byte_len);
	initLock(&pid_pool.lock);
}

pid_t createPid(void)
{
	acquireLock(&pid_pool.lock);
	int bite_index = 0;
	for(;bite_index<MAX_PROCESS_CNT;bite_index++){
		if(bitIsUsed(&pid_pool.bitmap,bite_index)==false){
			setBitmap(&pid_pool.bitmap,bite_index,1);
			releaseLock(&pid_pool.lock);
			return bite_index+1;
		}
	}
	releaseLock(&pid_pool.lock);
	return -1;
}

void removePid(pid_t pid)
{
	acquireLock(&pid_pool.lock);
	ASSERT(bitIsUsed(&pid_pool.bitmap,pid-1)==true);
	setBitmap(&pid_pool.bitmap,pid-1,0);
	releaseLock(&pid_pool.lock);
}

/*系统空闲时运行的线程*/
static void idle(void* arg UNUSED)
{
	while(1){
		thread_block(TASK_BLOCKED);
		asm volatile("sti; hlt;" ::: "memory");
	}
}

/* threade function booter */
static void execFunc(thread_func func,void*args)
{
	enableInt();
	func(args);
}
/*
static void threadRecoverer(void)
{
	closeInt();
	struct task_struct* cur_pcb = getpcb();
	cur_pcb->status = TASK_DIED;
	schedule();
}*/

/*init task_srtuct base data*/
static void initThreadBase(struct task_struct* thread,const char*name,int prio)
{
	ASSERT(thread != NULL);

	memset(thread,0,sizeof(struct task_struct));
	thread->self_kernel_stack = ((uint32_t)thread+PG_SIZE);
	thread->status=TASK_READY;
	strcpy(thread->name,name);
	thread->priority = prio;
	thread->ticks = prio;
	thread->elapsed_ticks = 0;
	thread->pgdir_vaddr = NULL;
	thread->fd_table[0]=0;
	thread->fd_table[1]=1;
	thread->fd_table[2]=2;
	int i = 3;
	for(;i<MAX_FILES_OPEN_PER_PROCESS;i++){
		thread->fd_table[i]=-1;
	}
	thread->cwd_inode_no = 0;
	thread->cur_part = getPart("/");
	thread->stack_magic = 0x20040104;
}
/*init task_struct stack data*/
static void initThreadStack(struct task_struct*thread,thread_func func,void*args)
{
	ASSERT(thread!=NULL);

	thread->self_kernel_stack -= (sizeof(struct intr_stack));
	thread->self_kernel_stack -= (sizeof(struct thread_stack));
	struct thread_stack* thread_stack = (struct thread_stack*)(thread->self_kernel_stack);
	thread_stack->eip = &execFunc;
	thread_stack->retaddr = NULL;
	thread_stack->function = func;
	thread_stack->func_args = args;
	thread_stack->ebp=thread_stack->ebx=thread_stack->edi=thread_stack->esi=0;
}

/*create new thread*/
struct task_struct* thread_start(char* name,int prio,thread_func func,void* args)
{
	struct task_struct* thread = (struct task_struct*)mallocKernelPage(1);
	if(thread == NULL) return NULL;
	memset(thread,0,sizeof(struct task_struct));
	initThreadBase(thread,name,prio);
	initThreadStack(thread,func,args);
	initMemBlockDesc(thread->descs);
	enum int_status stat = closeInt();
	ASSERT(find_elem(&thread_ready_list,&thread->ready_node)==false);
	list_append(&thread_ready_list,&thread->ready_node);

	ASSERT(find_elem(&thread_all_list,&thread->all_node)==false);
	list_append(&thread_all_list,&thread->all_node);
	setIntStatus(stat);
	return thread;
}

struct task_struct* getpcb(void)
{
	int esp;	
	asm volatile("mov %%esp,%0":"=g"(esp));
	return (struct task_struct*)(esp&0xfffff000);
}

//init kernel main
static void make_main_thread(void)
{
	struct task_struct* main_thread = getpcb();
	initThreadBase(main_thread,"main",31);
	main_thread->status = TASK_RUNNING;
	main_thread->self_kernel_stack = 0xc009f000;
	ASSERT(!find_elem(&thread_all_list,&main_thread->all_node));
	list_append(&thread_all_list,&main_thread->all_node); 
	thread_tag = &main_thread->ready_node;
	main_thread->pid = 0;
}


/*
 *Description: get current PCB and judge the status of PCB 
 *status:(1)running time end (2)blocked (3)died
 * */
void schedule(void)
{
	enum int_status stat = closeInt();
	//ASSERT(getIntStatus()==INT_OFF); 
	struct task_struct* cur_pcb = getpcb();     //get current pcb 
	if(cur_pcb -> status == TASK_RUNNING){	    // cur_pcb->ticks == 0
		ASSERT(find_elem(&thread_ready_list,&cur_pcb->ready_node)==false);
		list_append(&thread_ready_list,&cur_pcb->ready_node);
		cur_pcb -> ticks = cur_pcb->priority;
		cur_pcb -> status = TASK_READY;
	}

	if(list_empty(&thread_ready_list)){ /*若没有可执行的线程则唤醒idle线程*/
		thread_unblock(idle_thread);
	}
	thread_tag =list_pop(&thread_ready_list);                  			  //get first process's node form pcb queue
	struct task_struct*next_pcb = (struct task_struct*)elem2entry(struct task_struct,ready_node,thread_tag);
	activateProcess(next_pcb);
	//put_str("switch thread : form");put_str(cur_pcb->name);put_char(' ');put_int(cur_pcb->pid);put_char(' ');put_str("to");put_str(next_pcb->name);put_int(next_pcb->pid);put_char('\n');
	next_pcb -> status = TASK_RUNNING;
	/*if (cur_pcb -> status == TASK_DIED) 
		switch_to_and_free(cur_pcb,next_pcb);
	else*/ 
	switch_to(cur_pcb,next_pcb);                     // save old esp and switch to new esp
	setIntStatus(stat);
}

/*
 *Description: init main_thread , thread_ready_list , thread_all_list
 * */
void initThread(void)
{
	put_str("init thread start\n");
	list_init(&thread_ready_list);
	list_init(&thread_all_list);
	initPidPool();
	make_main_thread();
	//executeProcess(init,"init");
	idle_thread = thread_start("idle",10,&idle,NULL); //初始化idle线程
	put_str("init thread done\n");
}

/*
 *Description: blocked current thread and schedule other thread (atomic operation)
 */
void thread_block(enum task_status stat)
{
	ASSERT(stat == TASK_BLOCKED || stat == TASK_WAITING || stat == TASK_HANGING);
	enum int_status old_int_status = closeInt();
	struct task_struct*cur_pcb = getpcb();	
	cur_pcb -> status = stat;
	//save all registers
	asm volatile("pushl %ds;pushl %es;pushl %fs;pushl %gs;pusha;");
	schedule();
	asm volatile("popa;popl %gs;popl %fs;popl %es;popl %ds");
	setIntStatus(old_int_status);
}
/*
 *Description: unblock a thread by its PCB (atomic opeartion)
 * */
void thread_unblock(struct task_struct*pcb)
{
	enum int_status old_int_status = closeInt();
	ASSERT(pcb->status == TASK_BLOCKED || pcb->status == TASK_WAITING || pcb->status == TASK_HANGING);
	if(pcb->status != TASK_READY){
		ASSERT(!find_elem(&thread_ready_list,&pcb->ready_node));
		list_push(&thread_ready_list,&pcb->ready_node); //阻塞队列加到队首以优先调用
		pcb->status = TASK_READY;
	}
	setIntStatus(old_int_status);
}

/*主动挂起当前进程 （不重新设置proi）*/
void thread_yield(void)
{
	struct task_struct * pcb = getpcb();
	enum int_status stat = closeInt();
	ASSERT(find_elem(&thread_ready_list,&pcb->ready_node)==false);
	list_append(&thread_ready_list,&pcb->ready_node);
	pcb -> status = TASK_READY;
	schedule();
	setIntStatus(stat);
}

static void pad_buf(char*buf,uint32_t buf_len,void* arg,const char mode)
{
	memset(buf,0,buf_len);
	uint32_t i = 0;
	switch (mode)
	{
		case 'x':
			i=sprintf(buf,"%x",*(uint32_t*)arg);
			break;
		case 'u':
			i=sprintf(buf,"%u",*(uint32_t*)arg);
			break;
		case 'd':
			i=sprintf(buf,"%d",*(uint32_t*)arg);
			break;
		case 's':
			i=sprintf(buf,"%s",(char*)arg);
			break;
	}
	for(;i<buf_len;i++){
		buf[i]=' ';
	}
	buf[buf_len-1]=0;
}

static void pad_print(struct list_elem*pelem,char*buf,uint32_t buf_len)
{
	struct task_struct * pcb = (struct task_struct*)elem2entry(struct task_struct,all_node,pelem);
	pad_buf(buf,buf_len,pcb->name,'s');
	sys_write(1,buf,buf_len);
	pad_buf(buf,buf_len,&pcb->pid,'u');
	sys_write(1,buf,buf_len);
	pad_buf(buf,buf_len,&pcb->parent_pid,'u');
	sys_write(1,buf,buf_len);
	switch(pcb->status){
		case TASK_RUNNING:
			pad_buf(buf,buf_len,"RUNNING\0",'s');
			break;
		case TASK_READY:
			pad_buf(buf,buf_len,"TASK_READY\0",'s');
			break;
		case TASK_BLOCKED:
			pad_buf(buf,buf_len,"TASK_BLOCKED\0",'s');
			break;
		case TASK_WAITING:
			pad_buf(buf,buf_len,"WAITTING\0",'s');
			break;
		case TASK_HANGING:
			pad_buf(buf,buf_len,"HANGING\0",'s');
			break;
		case TASK_DIED:
			pad_buf(buf,buf_len,"DIED\0",'s');
			break;
	}
	sys_write(1,buf,strlen(buf));
	pad_buf(buf,buf_len,&pcb->ticks,'u');
	sys_write(1,buf,strlen(buf));
	sys_write(1,"\n",1);
}

void sys_ps(void)
{
	const char* str = "COMMAND        PID           PPID             STATU           TICKS\n\0";
	sys_write(1,str,strlen(str));
	struct list_elem* pelem = thread_all_list.head.next;
	char buf[16]={0};
	while(pelem!=&thread_all_list.tail){
		pad_print(pelem,buf,16);
		pelem=pelem->next;
	}
}
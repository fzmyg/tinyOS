#include"sync.h"
#include"string.h"
#include"interrupt.h"
#include"debug.h"
#include"thread.h"
#include"print.h"
/*
 * 初始化信号量
 * */
void initSema(struct semaphore* psemaphore,uint8_t val)
{
	psemaphore -> value = val;
	list_init(&psemaphore -> waiters);   // init waiters list
}

/*
 *Description:初始化锁的信号量，拥有者，拥有者获取次数
 * */
void initLock(struct lock* plock)
{
	plock -> holder = NULL;
	plock -> holder_repeat_nr = 0;
	initSema(&plock->semaphore,1);       // init semaphore's value to 1
}
/*
 * 获取信号量（关中断条件下进行）
 * */
static void semaDown(struct semaphore * psema)
{
	enum int_status old_status = closeInt();
	while(psema->value == 0){
		struct task_struct* cur_pcb = getpcb();
		ASSERT(find_elem(&psema->waiters,&cur_pcb->ready_node)==false);
		list_append(&psema->waiters,&cur_pcb->ready_node);	//add current pcb to semaphore's waiters list
		thread_block(TASK_BLOCKED);			  //阻塞当前队列
	}
	psema->value --;
	ASSERT(psema->value == 0);
	setIntStatus(old_status);
}

/*
 *释放信号量
 * */
static void semaUp(struct semaphore*psema)
{
	enum int_status old_status = closeInt();
	ASSERT(psema->value == 0);
	if(!list_empty(&psema->waiters)){
		struct task_struct*thread_blocked = (struct task_struct*)elem2PCBentry(struct task_struct,ready_node,list_pop(&psema->waiters));//从等待进程中获取进程PCB
		thread_unblock(thread_blocked);		//将阻塞的进程加入到线程就绪队列
	}
	psema->value++;	
	ASSERT(psema->value == 1);
	setIntStatus(old_status);
}

/*
 * 获取锁 （开中断）
 * */
void acquireLock(struct lock* plock)
{
	if(plock->holder != getpcb()){   //自己不是锁拥有者
		semaDown(&plock->semaphore); //获取信号量，可能会阻塞当前进程
		plock->holder = getpcb();
		ASSERT(plock->holder_repeat_nr == 0);
		plock->holder_repeat_nr = 1;
	}else{ //是锁拥有者
		plock->holder_repeat_nr++; //重复获取锁数增加
	}
}
/*
 * 释放锁 （开中断）
 * */
void releaseLock(struct lock*plock)
{
	ASSERT(plock->holder == getpcb());
	if(plock -> holder_repeat_nr > 1){ //重复申请过锁
		plock->holder_repeat_nr --;    //申请锁数量降低
		return ;
	}//未重复申请过锁
	ASSERT(plock->holder_repeat_nr == 1);
	plock -> holder = NULL;		//必须在调用semaUp前将holder置空，避免其他进程获取锁后holder被设置为NULL，然后该进程再次获取锁后将自己阻塞造成死锁
	plock -> holder_repeat_nr = 0;
	semaUp(&plock->semaphore);	//释放信号量
}



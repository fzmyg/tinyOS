#include"sync.h"
#include"string.h"
#include"interrupt.h"
#include"debug.h"
#include"thread.h"
#include"print.h"
/*
 * Description: init semaphore's value and waiters_list
 * Return Value: empty
 * */
void initSema(struct semaphore* psemaphore,uint8_t val)
{
	psemaphore -> value = val;
	list_init(&psemaphore -> waiters);   // init waiters list
}

/*
 *Description: init  semaphore and holder and holder_repeat_nr
 *Return Value:empty
 * */
void initLock(struct lock* plock)
{
	plock -> holder = NULL;
	plock -> holder_repeat_nr = 0;
	initSema(&plock->semaphore,1);       // init semaphore's value to 1
}
/*
 *Description:decline semaphore's value under int_off(atomic operation)
 *Return Value: empty
 * */
static void semaDown(struct semaphore * psema)
{
	enum int_status old_status = closeInt();
	while(psema->value == 0){
		struct task_struct* cur_pcb = getpcb();
		ASSERT(find_elem(&psema->waiters,&cur_pcb->ready_node)==false);
		list_append(&psema->waiters,&cur_pcb->ready_node);	// add current pcb to semaphore's waiters list
		thread_block(TASK_BLOCKED);				// block current thread and schedule other thread
	}
	psema->value --;
	ASSERT(psema->value == 0);
	setIntStatus(old_status);
}

/*
 *Description: add semaphore's value under int_off(atomic operation)
 *Return Value: empty
 * */
static void semaUp(struct semaphore*psema)
{
	enum int_status old_status = closeInt();
	ASSERT(psema->value == 0);
	if(!list_empty(&psema->waiters)){
		struct task_struct*thread_blocked = (struct task_struct*)elem2PCBentry(struct task_struct,ready_node,list_pop(&psema->waiters));
		thread_unblock(thread_blocked);		// pop head of waiters's queue and wake up the thread(not execute immediately)
	}
	psema->value++;	
	ASSERT(psema->value == 1);
	setIntStatus(old_status);
}

/*
 *Description: acquire lock under int_on
 *Return Value: empty
 * */
void acquireLock(struct lock* plock)
{
	if(plock->holder != getpcb()){
		semaDown(&plock->semaphore); //wait for semaphore's value euqie to 1 and sub to 0
		plock->holder = getpcb();
		ASSERT(plock->holder_repeat_nr == 0);
		plock->holder_repeat_nr = 1;
	}else{
		plock->holder_repeat_nr++;
	}
}
/*
 *Description: release lock under int_on
 *Return Value: empty
 * */
void releaseLock(struct lock*plock)
{
	ASSERT(plock->holder == getpcb());
	if(plock -> holder_repeat_nr > 1){
		plock->holder_repeat_nr --;
		return ;
	}
	ASSERT(plock->holder_repeat_nr == 1);
	plock -> holder = NULL;		//must behind call semaUp(); otherwise when other thread get same lock,a died lock will be cased;
	plock -> holder_repeat_nr = 0;
	semaUp(&plock->semaphore);	
}



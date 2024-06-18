#include"ioqueue.h"
#include"debug.h"
#include"interrupt.h"
#include"string.h"
enum waiter_flags{
	CONSUMER,PRODUCER
};
void initIOQueue(struct ioqueue*pq)
{
	memset(pq,0,sizeof(struct ioqueue));
	initLock(&pq->lock);	
}

static uint32_t nextPos(uint32_t pos)
{
	return (pos+1)%BUFSIZE;
}

bool isIOQueueFull(const struct ioqueue*pq)
{
	return nextPos(pq->write_index) == pq->read_index;	
}

bool isIOQueueEmpty(const struct ioqueue*pq)
{
	return pq->write_index == pq->read_index;
}

static void ioq_wait(struct ioqueue*ioq,enum waiter_flags wf)
{
	struct task_struct** ppcb = wf==CONSUMER?&(ioq->consumer_waiter):&(ioq->producer_waiter);
	acquireLock(&ioq->lock);
	ASSERT(*ppcb==NULL);
	*ppcb = getpcb();
	releaseLock(&ioq->lock);
	thread_block(TASK_BLOCKED);
}

static void ioq_wakeup(struct ioqueue* ioq,enum waiter_flags wf)
{
	struct task_struct** ppcb = wf==CONSUMER?&(ioq->consumer_waiter):&(ioq->producer_waiter);
	struct task_struct* pcb_wake = NULL;
	acquireLock(&ioq->lock);
	ASSERT(*ppcb!=NULL);
	pcb_wake = *ppcb;
	*ppcb = NULL;
	releaseLock(&ioq->lock);
	thread_unblock(pcb_wake);
}

char ioq_get_char(struct ioqueue*pq)
{
	while(isIOQueueEmpty(pq)){//队列为空，将当前进程阻塞并加入到消费者等待队列
		ioq_wait(pq,CONSUMER);
	}
	acquireLock(&pq->lock);
	char ch = pq -> buf[pq->read_index];
	pq->read_index = nextPos(pq->read_index);
	if(pq->producer_waiter!=NULL) 
		ioq_wakeup(pq,PRODUCER);//唤醒等待的生产者
	releaseLock(&pq->lock);
	return ch;
}

void ioq_put_char(struct ioqueue*pq,char ch)
{
	while(isIOQueueFull(pq)){
		ioq_wait(pq,PRODUCER);  //block current thread
	}
	acquireLock(&pq->lock);
	pq->buf[pq->write_index]=ch;
	pq->write_index = nextPos(pq->write_index);
	if(pq->consumer_waiter != NULL)
		ioq_wakeup(pq,CONSUMER);
	releaseLock(&pq->lock);
}


#include"ioqueue.h"
#include"debug.h"
#include"interrupt.h"
#include"string.h"
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

static void ioq_wait(struct task_struct** pthread)
{
	ASSERT(pthread!=NULL&&*pthread==NULL);
	*pthread = getpcb();
	thread_block(TASK_BLOCKED);
}

static void ioq_wakeup(struct task_struct**pthread)
{
	ASSERT(pthread!=NULL&&*pthread!=NULL);
	thread_unblock(*pthread);
	*pthread = NULL;
}

char ioq_get_char(struct ioqueue*pq)
{
	enum int_status stat = closeInt();
	while(isIOQueueEmpty(pq)){
		acquireLock(&pq->lock);
		ioq_wait(&pq->consumer_waiter);
		releaseLock(&pq->lock);
	}
	
	char ch = pq -> buf[pq->read_index];
	pq->read_index = nextPos(pq->read_index);
	if(pq->producer_waiter!=NULL) 
		ioq_wakeup(&pq->producer_waiter);
	setIntStatus(stat);
	return ch;
}
void ioq_put_char(struct ioqueue*pq,char ch)
{
	enum int_status stat = closeInt();
	while(isIOQueueFull(pq)){
		acquireLock(&pq->lock);
		ioq_wait(&pq->producer_waiter);  //block current thread
		releaseLock(&pq->lock);
	}
	
	pq->buf[pq->write_index]=ch;
	pq->write_index = nextPos(pq->write_index);
	if(pq->consumer_waiter != NULL)
		ioq_wakeup(&pq->consumer_waiter);
	setIntStatus(stat);
}


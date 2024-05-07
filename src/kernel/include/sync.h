#ifndef __SYNC_H__
#define __SYNC_H__
#include"list.h"
#include"stdint.h"
#include"thread.h"

struct semaphore{
	uint8_t value;
	struct list waiters;
};

struct lock{
	struct task_struct* holder;  		// lock's owner
	struct semaphore  semaphore;		// two tupe semaphore
	uint32_t holder_repeat_nr;		// the number of lock's owner apply for lock
};

extern void initSema(struct semaphore*semaphore,uint8_t value);

extern void initLock(struct lock*plock);

extern void acquireLock(struct lock*plock);

extern void releaseLock(struct lock*plock);

#endif

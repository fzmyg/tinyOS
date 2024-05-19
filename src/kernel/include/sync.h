#ifndef __SYNC_H__
#define __SYNC_H__
#include"list.h"
#include"stdint.h"
#include"thread.h"

/* 信号量*/
struct semaphore{
	uint8_t value;  	//信号量值 --- 一般设为二元信号量
	struct list waiters;//信号量等待队列
};

struct lock{
	struct task_struct* holder;  	//锁拥有者
	struct semaphore  semaphore;	//锁信号量
	uint32_t holder_repeat_nr;		//锁拥有者申请锁次数
};

/*初始化信号量*/
extern void initSema(struct semaphore*semaphore,uint8_t value);
/*初始化锁*/
extern void initLock(struct lock*plock);
/*获取锁*/
extern void acquireLock(struct lock*plock);
/*释放锁*/
extern void releaseLock(struct lock*plock);
/*信号量增加*/
extern void semaUp(struct semaphore*sema);
/*信号量降低*/
extern void semaDown(struct semaphore*sema);
#endif

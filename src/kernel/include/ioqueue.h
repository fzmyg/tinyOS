#ifndef __IOQUEUE_H__
#define __IOQUEUE_H__
#include"sync.h"   //struct lock
#include"thread.h" //struct task_struct
#include"stdint.h" //int type

#define BUFSIZE 2000
struct  ioqueue{
	struct lock lock;
	struct task_struct* producer_waiter;
	struct task_struct* consumer_waiter;
	char buf[BUFSIZE];
	uint32_t write_index;	
	uint32_t read_index;
};

extern void initIOQueue(struct ioqueue*pq);

extern bool isIOQueueFull(const struct ioqueue*pq);

extern bool isIOQueueEmpty(const struct ioqueue*pq);

extern char ioq_get_char(struct ioqueue*pq);

extern void ioq_put_char(struct ioqueue*pq,char ch);
#endif

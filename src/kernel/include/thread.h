#ifndef __THREAD_H__
#define __THREAD_H__

#include"stdint.h"
#include"list.h"
#include"memory.h"

typedef void*(*thread_func)(void*);
typedef uint16_t pid_t;

enum task_status{
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIED
};
/* reserve to process boot */
typedef struct intr_stack{
	uint32_t vec_no;
	uint32_t edi;	
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy;
	uint32_t ebx;	
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t gs;	
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	
	uint32_t err_code;
	void* eip;
	uint32_t cs;
	uint32_t eflags;
	void*esp;
	uint32_t ss;
}intrStack;

/* reserve to thread boot */
typedef struct thread_stack{
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;
	
	void(*eip)(thread_func fun,void*args);
	void* unused_retaddr;
	thread_func function;
	void*func_args;
}threadStack;

typedef struct task_struct{
	uint32_t self_kernel_stack; 		// each process has its own kernel stack
	pid_t pid;
	enum task_status status;		// status of process
	char name[16];				// process name
	uint8_t priority;			// priority of process
	uint8_t ticks;				// when 0x20 interrupt occur ,ticks -= 1
	uint32_t elapsed_ticks;			// when 0x20 interrupt occur ,elapsed_ticks += 1
	struct list_elem ready_node;		// ready node to link with all ready process's PCB
	struct list_elem all_node;	 	// all node to link with all PCB
	uint32_t pgdir_vaddr; 			// process's own page directory table's kernel virtual addr
	struct vmpool vaddr_pool;		// vaddr_pool
	uint32_t stack_magic;			// magic number to protect task_struct 
}taskStruct;

extern struct task_struct* thread_start (char*name,int proi,thread_func func,void*argv);

extern struct task_struct* getpcb(void);

extern void schedule(void);

extern void initThread(void);

extern void switch_to(struct task_struct*,struct task_struct*);

extern void switch_to_and_free(struct task_struct*,struct task_struct*);

extern void thread_block(enum task_status stat);

extern void thread_unblock(struct task_struct*pcb);



#endif

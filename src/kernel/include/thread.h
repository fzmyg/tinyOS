#ifndef __THREAD_H__
#define __THREAD_H__

#include"stdint.h"
#include"list.h"
#include"memory.h"
#define MAX_FILES_OPEN_PER_PROCESS 8

typedef void(*thread_func)(void*);
typedef int16_t pid_t;

enum task_status{
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIED
};
/* 用户进程设置栈，从栈中进入特权级3 */
struct intr_stack{
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
	void* eip;         //用户进程执行函数
	uint32_t cs;       //用户进程代码段选择子
	uint32_t eflags;   //中断前标志寄存器
	void*esp;          //用户进程栈
	uint32_t ss;	   //用户进程栈段选择子
};

/* 进程初始化设置 switch_to 进入 function */
struct thread_stack{
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;
	
	void(*eip)(thread_func fun,void*args);
	void* retaddr;  	//线程回收器地址
	thread_func function;
	void*func_args;
};

struct task_struct{
	uint32_t self_kernel_stack; 	//进程内核栈地址---esp0
	pid_t pid;						//进程ID
	pid_t parent_pid;				//父进程ID
	enum task_status status;		//线程/进程ID
	char name[16];					//线程/进程名
	uint8_t priority;				//线程/进程 权重值(总时间片)
	uint8_t ticks;					//线程/进程 剩余时间片
	uint32_t elapsed_ticks;			//进程总共消耗的时间片
	struct list_elem ready_node;	//就绪队列节点
	struct list_elem all_node;	 	//全部队列节点
	uint32_t pgdir_vaddr; 			//页目录表在内核进程页表中的地址
	struct vmpool vaddr_pool;		//进程虚拟内存池
	struct mem_block_desc descs[MEM_DESC_CNT]; //用户内存管理
	int32_t fd_table[MAX_FILES_OPEN_PER_PROCESS]; //文件描述符数组
	uint32_t cwd_inode_no;          //工作目录inode号
	uint32_t stack_magic;			//魔术 --- 防止内核栈溢出 
};

extern struct list thread_ready_list,thread_all_list;

/*创建内核线程*/
extern struct task_struct* thread_start (char*name,int proi,thread_func func,void*argv);

/*获取当前进程/线程PCB*/
extern struct task_struct* getpcb(void);

/*进程调度*/
extern void schedule(void);

/*初始化内main内核线程*/
extern void initThread(void);

/*线程切换*/
extern void switch_to(struct task_struct*,struct task_struct*);

/*线程切换并释放内存池*/
extern void switch_to_and_free(struct task_struct*,struct task_struct*);

/*阻塞线程*/
extern void thread_block(enum task_status stat);

/*唤醒线程*/
extern void thread_unblock(struct task_struct*pcb);

/*挂起线程*/
extern void thread_yield(void);

/*获取pid*/
pid_t createPid(void);

/*销毁pid*/
void removePid(pid_t pid);

#endif

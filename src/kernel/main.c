#include"print.h"
#include"init.h"
#include"debug.h"
#include"string.h"
#include"memory.h"
#include"thread.h"
#include"interrupt.h"
#include"console.h"
#include"ioqueue.h"
#include"keyboard.h"
#include"process.h"
#include"stdio.h"
void* threadA(void*s);
void* threadB(void*s);
void* userProcessA(void);
int main(void)
{
	cls(); 
	put_str("booting kernel\n");
	init_all();
	//thread_start("kernel_thread_a",31,&threadA,"A  ");
	//thread_start("kernel_thread_b",31,&threadA,"B  ");
	enableInt();
	executeProcess(userProcessA,"user process A");
	while(1){};
	return 0;
}

void* threadA(void*s)
{
	while(1){
		
		enum int_status  stat = closeInt();
		if(!isIOQueueEmpty(&kbd_buf)){
			console_put_str(s);
			char ch = ioq_get_char(&kbd_buf);
			console_put_char(ch);
		}
		setIntStatus(stat);
	}
	return NULL;
}

void* threadB(void*s)
{
	int ans = 1000;
	for(;ans>0;ans--){
		console_put_str("ans");
	}
	return NULL;
}

void* userProcessA(void)
{
	int val = 1;
	char s[]="zbcdawdwad";
	val=(uint32_t)getpid();
	while(1){
		void*p = malloc(2);
		printf("zbcsb%d%s%x\n",0,s,(uint32_t)p);
	}
	return NULL;
}

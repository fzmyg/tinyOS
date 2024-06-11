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
#include"syscall.h"
#include"syscall_init.h"
#include"fs.h"
#include"file.h"
#include"stdiok.h"
#define UNUSED __attribute__((unused))

void threadA(void*s);
void threadB(void*s);
void* userProcessA(void);
int main(void)
{
	cls(); 
	put_str("booting kernel\n");
	init_all();
	char buf[12]={0};
	if(sys_getcwd(buf,12)==-1){
		printk("error");
	}
	sys_mkdir("/home");
	struct file_stat stat;
	sys_stat("/home",&stat);
	printf("/home : inode_no:%d type:%d size:%d\n",stat.st_inode_no,stat.st_ft,stat.st_size);
	printk("%s",buf);
	while(1){};
	return 0;
}

void threadA(void*s)
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
}

void threadB(void*s UNUSED)
{
	int ans = 1000;
	for(;ans>0;ans--){
		console_put_str("ans");
	}
}

void* userProcessA(void)
{
	int val = 1;
	char s[]="zbcdawdwad";
	val=(uint32_t)getpid();
	while(1){
		void*p = malloc(2);
		printf("zbcsb%d%s%x\n",0,s,(uint32_t)p);
		free(p);
	}
	return NULL;
}

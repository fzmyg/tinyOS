#include"init.h"
#include"interrupt.h"
#include"timer.h"
#include"print.h"
#include"memory.h"
#include"thread.h"
#include"console.h"
#include"keyboard.h"
#include"tss.h"
#include"syscall_init.h"
#include"ide.h"
#include"fs.h"
void init_all(void)
{
	put_str("init_all start\n");
	idt_init(); 	//初始化中断描述符
	initMemPool();	//初始化内存池
	initThread();	//初始化现车给
	initTimer();	//初始化计数器
	initConsole();	//初始化输出锁
	initKeyBoard(); //初始化键盘程序
	initTss();		//初始化tss
	initSyscall();	//初始化系统调用
	enableInt();	
	initIDE();		//初始化硬盘
	initFileSystem();//初始化文件系统
	put_str("init all done\n");
}

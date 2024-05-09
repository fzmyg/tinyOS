#include"init.h"
#include"interrupt.h"
#include"timer.h"
#include"print.h"
#include"memory.h"
#include"thread.h"
#include"console.h"
#include"keyboard.h"
#include"tss.h"
#include"syscall.h"
void init_all(void)
{
	put_str("init_all start\n");
	idt_init();
	initMemPool();
	initThread();
	initTimer();
	initConsole();
	initKeyBoard();
	initTss();
	initSyscall();
	put_str("init all done\n");
}

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
void init_all(void)
{
	put_str("init_all start\n");
	ide_init();
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

#include"console.h"
#include"sync.h"
#include"print.h"
#include"debug.h"
#include"interrupt.h"
static struct lock console_lock;

void initConsole(void)
{
	initLock(&console_lock);
}

void consoleAqeLock(void)
{
	acquireLock(&console_lock);
}

void consoleRleLock(void)
{
	releaseLock(&console_lock);
}

void console_put_str(const char*s)
{
	consoleAqeLock();
	put_str(s);
	consoleRleLock();
}

void console_put_char(int ch)
{
	consoleAqeLock();
	put_char(ch);
	consoleRleLock();
}

void console_put_int(int val)
{
	consoleAqeLock();
	put_int(val);
	consoleRleLock();
}


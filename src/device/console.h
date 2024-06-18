#ifndef __CONSOLE_H__
#define __CONSOLE_H__

extern void initConsole(void);

extern void consoleAqeLock(void);

extern void consoleRleLock(void);

extern void console_put_str(const char*s);

extern void console_put_char(int ch);

extern void console_put_int(int val);

#endif

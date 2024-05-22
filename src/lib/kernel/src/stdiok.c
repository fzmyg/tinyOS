#include"stdiok.h"
#include"stdio.h"
#include"console.h"
#include"string.h"
uint32_t printk(const char* format,...)
{
    va_list arg;
    va_start(arg,format);

    char buf[1024];
    uint32_t len = vsprintf(buf,format,arg);
    va_end(arg);
    console_put_str(buf);
    return len;
}
#include"debug.h"
#include"interrupt.h"
#include"print.h"
void panic_spin(const char*file_name,int line_number,const char*func_name,const char*condition)
{
        closeInt();
        put_str("######error!#####\n");
        put_str("file_name:");put_str(file_name);put_char('\n');
        put_str("line_number:");put_int(line_number);put_char('\n');
        put_str("func_name:");put_str(func_name);put_char('\n');
        put_str("assert condition:");put_str(condition);put_char('\n');
        while(1);
}


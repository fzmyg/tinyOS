#include"stdio.h"
#include"assert.h"
void panic_spin(const char*file,const int line_number,const char*func,const char*condition)
{
    printf("######error!#####\n");
    printf("file_name:%s\n",file);
    printf("line_number:%d\n",line_number);
    printf("func_name:%s\n",func);
    printf("assert condition:%s\n",condition);
}
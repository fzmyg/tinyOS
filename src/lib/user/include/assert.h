#ifndef __ASSERT_H__
#define __ASSERT_H__

extern void panic_spin(const char*file,const int line_number,const char*func,const char*condition);
#define ASSERT(condition) if((condition)==0){\
                            PANIC(__FILE__,__LINE__,__FUNC__,#condition);}
#define PANIC(file,line,func,condition) panic_spain(file,line,func,condition);


#endif
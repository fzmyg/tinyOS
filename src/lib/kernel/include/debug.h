#ifndef __DEBUG_H__
#define __DEBUG_H__

extern void panic_spin(const char*file_name,int line_number,const char*func_name,const char*condition);

#define PANIC(...) panic_spin(__FILE__,__LINE__,__func__,__VA_ARGS__)

#ifdef NDEBUG
	#define ASSERT(CONDITION) ((void)0)
#else
	#define ASSERT(CONDITION) \
		if(CONDITION){}	 \
		else{PANIC(#CONDITION);}
#endif

#endif

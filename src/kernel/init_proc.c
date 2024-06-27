#include"stdint.h"
#include"syscall.h"
#include"shell.h"
extern void init_proc(void);
void init_proc(void)
{
	uint32_t ret_pid = fork();
	if(ret_pid==0){
		shell();
	}else{
		int stat;
		while(1){
			wait(&stat);
		}
	}
	while(1);
}

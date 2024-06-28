#include"print.h"
#include"init.h"
#include"debug.h"
#include"string.h"
#include"memory.h"
#include"thread.h"
#include"interrupt.h"
#include"console.h"
#include"ioqueue.h"
#include"keyboard.h"
#include"process.h"
#include"stdio.h"
#include"syscall_init.h"
#include"fs.h"
#include"file.h"
#include"stdiok.h"
#define UNUSED __attribute__((unused))
extern void init_proc(void);
int main(void)
{
	cls(); 
	put_str("booting kernel\n");
	init_all();
	uint32_t file_size = 29 * 1024;
	uint32_t file_sector_cnt = (file_size-1)/512 + 1;
	char* buf = sys_malloc(file_size);
	if(buf==NULL){
		PANIC("malloc error");
		while(1);
	}
	struct disk* sda = &channels[0].disks[0];
	readDisk(buf,sda,400,file_sector_cnt);
	int fd = sys_open("vim",O_CREATE|O_RDWR);
	if(fd==-1){
		printk("create file error\n");
		while(1);
	}
	sys_write(fd,buf,file_size);
	sys_close(fd);
	executeProcess(init_proc,"init");
	
	while(1){};
	return 0;
}


#include"syscall.h"
#include"string.h"
#include"thread.h"
#include"syscall_init.h"
#define _SYSCALL0(NUMBER) ({int ret_val = 0;\
                            asm volatile("int $0x80":"=a"(ret_val):"a"(NUMBER):"memory");\
                            ret_val;})
#define _SYSCALL1(NUMBER,ARG1) ({int ret_val = 0;\
                            asm volatile("int $0x80":"=a"(ret_val):"a"(NUMBER),"b"(ARG1):"memory");\
                            ret_val;})
#define _SYSCALL2(NUMBER,ARG1,ARG2) ({int ret_val = 0;\
                            asm volatile("int $0x80":"=a"(ret_val):"a"(NUMBER),"b"(ARG1),"c"(ARG2):"memory");\
                            ret_val;})
#define _SYSCALL3(NUMBER,ARG1,ARG2,ARG3) ({int ret_val = 0;\
                            asm volatile("int $0x80":"=a"(ret_val):"a"(NUMBER),"b"(ARG1),"c"(ARG2),"d"(ARG3):"memory");\
                            ret_val;})

pid_t getpid(void)
{
    return (pid_t)_SYSCALL0(GET_PID);
}

void clear(void)
{
    _SYSCALL0(CLR_SCREEN);
}

void* malloc(uint32_t size)
{
    return (void*)_SYSCALL1(MALLOC,size);
}

void free(void*ptr)
{
    _SYSCALL1(FREE,ptr);
}

int open(const char* path,uint32_t o_mode)
{
    return _SYSCALL2(OPEN,path,o_mode);
}

void close(int fd)
{
    _SYSCALL1(CLOSE,fd);
}

int write(int fd,const char* buf,uint32_t count)
{
    return _SYSCALL3(WRITE,fd,buf,count);
}

int read(int fd,char*buf,uint32_t count)
{
    return _SYSCALL3(READ,fd,buf,count);
}

int lseek(int fd,int off,uint32_t whence)
{
    return _SYSCALL3(LSEEK,fd,off,whence);
}

/*删除文件*/
int unlink(const char* file_name)
{
    return _SYSCALL1(UNLINK,file_name);
}
/*创建目录*/
int mkdir(const char* path)
{
    return _SYSCALL1(MKDIR,path);
}
/*删除目录*/
int rmdir(const char* path)
{
    return _SYSCALL1(RMDIR,path);    
}

/*打开目录*/
struct dir* opendir(const char* path)
{
    return (struct dir*)_SYSCALL1(OPENDIR,path);
}
/*关闭目录*/
void closedir(struct dir* dir)
{
    _SYSCALL1(CLOSEDIR,dir);
}
/*读取目录项*/
struct dir_entry* readdir(struct dir* dir)
{
    return (struct dir_entry*)_SYSCALL1(READDIR,dir);
}
/*设置目录指针*/
void rewinddir(struct dir*dir)
{
    _SYSCALL1(REWINDDIR,dir);
}
/*获取当前工作目录*/
int getcwd(char*buf,unsigned int size)
{
    return _SYSCALL2(GETCWD,buf,size);
}
/*更改工作目录*/
int chdir(const char* path)
{
    return _SYSCALL1(CHDIR,path);
}

int stat(const char*file_path,struct file_stat* stat)
{
    return _SYSCALL2(STAT,file_path,stat);
}

pid_t fork(void)
{
    return _SYSCALL0(FORK);
}

int ps(void)
{
    return _SYSCALL0(PS);
}
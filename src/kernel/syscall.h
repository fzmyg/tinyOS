#ifndef __SYS_CALL__
#define __SYS_CALL__
#include"stdint.h"
#include"thread.h"
#include"fs.h"
/*获取进程ID*/
extern pid_t getpid(void);
/*清屏*/
extern void clear(void);
/*项屏幕输出*/
extern uint32_t print(const char*s);
/*申请内存*/
extern void* malloc(uint32_t size);
/*回收内存*/
extern void free(void*ptr);
/*打开文件*/
extern int open(const char* path,uint32_t o_mode);
/*关闭文件*/
extern void close(int fd);
/*写文件*/
extern int write(int fd,const char* buf,uint32_t count);
/*读文件*/
extern int read(int fd,char* buf,uint32_t count);
/*更改文件指针*/
extern int lseek(int fd,int off,uint32_t whence);
/*删除文件*/
extern int unlink(const char* file_name);
/*创建目录*/
extern int mkdir(const char* path);
/*删除目录*/
extern int rmdir(const char* path);
/*打开目录*/
extern struct dir* opendir(const char* path);
/*关闭目录*/
extern void closedir(struct dir* dir);
/*读取目录项*/
extern struct dir_entry* readdir(struct dir* dir);
/*设置目录指针*/
extern void rewinddir(struct dir*dir);
/*获取当前工作目录*/
extern int getcwd(char*buf,unsigned int size);
/*更改工作目录*/
extern int chdir(const char* path);
/*查看文件属性*/
extern int stat(const char*file_path,struct file_stat*stat);
/*复制进程*/
extern pid_t fork(void);
/*打印全部进程信息*/
extern int ps(void);
#endif
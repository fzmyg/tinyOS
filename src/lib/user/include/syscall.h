#ifndef __SYS_CALL__
#define __SYS_CALL__
#include"stdint.h"
//#include"thread.h"
//#include"fs.h"
/*获取进程ID*/

typedef int32_t pid_t;

/*文件类型*/
enum file_type{
    FT_UNKNOW = 0 /*未知文件*/, FT_REGULAR = 1/*普通文件*/ , FT_DIRECTORY=2 /*目录*/
};
/*文件权限*/
enum privilege{
    PRI_EXEC=0,PRI_WRITE=1,PRI_READ=2
};
/*文件打开方式*/
enum oflags {
    O_RDONLY = 0, O_WRONLY=1,O_RDWR = 2,O_CREATE=4,O_APPEND=8
};
/*设置指针起始偏移*/
enum whence{
    SEEK_SET = 1,SEEK_CUR = 2,SEEK_END = 4
};

/**/
struct file_stat{
    uint32_t st_size;
    enum file_type st_ft;
    uint32_t st_inode_no;
};

struct dir{
    struct inode* inode;  //指向打开的inode
    uint32_t read_entry_cnt;     //暂时未使用
    uint8_t dir_buf[512]; //页目录数据缓冲区
};

/* 目录的inode指向的数据段*/
struct dir_entry{
    char name[16];                //名称
    uint32_t i_no;                //inode号
    enum file_type  f_type;      //文件类型
};



extern pid_t getpid(void);
/*清屏*/
extern void clear(void);
/*设置光标位置*/
extern void setcursor(int pos);
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
/*覆盖当前进程执行指定程序*/
extern int execv(const char* file_path,char*argv[]);
/**/
extern pid_t wait(int*status);

extern int exit(int status);

extern int dup(int fd);
#endif

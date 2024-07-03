#ifndef __FS_H__
#define __FS_H__
#include"ide.h"
#define MAX_INODE_PER_PARTS 4096    //分区最多创建文件数
#define SECTOR_SIZE 512             //扇区大小
#define BLOCK_SIZE 512              //块字节大小
#define MAX_PATH_LEN 512

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
/*搜索记录*/
struct searched_path_record{
    char searched_path[MAX_PATH_LEN];
    struct dir* parent_dir;
    enum file_type f_type;
    struct partition* part;
};
/**/
struct file_stat{
    uint32_t st_size;
    enum file_type st_ft;
    uint32_t st_inode_no;
};

/*为所有从盘创建文件系统*/
extern void initFileSystem(void);
//转换路径格式为标准格式
extern uint32_t convertPath(const char*in_path,char*out_path);
//获取路径深度
extern uint32_t getPathDepth(const char* path);
//打开文件
extern int32_t sys_open(const char* path_name,uint32_t o_mode);
//关闭文件
extern int32_t sys_close(int fd);
//写文件
extern int32_t sys_write(int fd,const char*buf,uint32_t count);
//读文件
extern int32_t sys_read(int fd,char*buf,uint32_t count);
//调整文件操作指针
extern int32_t sys_lseek(int fd,int off,uint32_t whence);
//删除文件
extern int32_t sys_unlink(const char* file_path);
//创建目录
extern int32_t sys_mkdir(const char* dir_path);
//打开目录
extern struct dir* sys_opendir(const char* dir_path);
//关闭目录
extern void sys_closedir(struct dir*dir);
//读取目录项
extern struct dir_entry* sys_readdir(struct dir*dir);
//设置目录读取指针
extern void sys_rewinddir(struct dir*dir);
//删除目录
extern int32_t sys_rmdir(const char* dir_path);
//获取当前工作目录
extern int sys_getcwd(char* path_buf,uint32_t size);
//改变工作目录
extern int sys_chdir(const char*path);

extern int sys_stat(const char*file_path,struct file_stat*stat);

#endif
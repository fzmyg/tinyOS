#ifndef __FILE_H__
#define __FILE_H__
#include"stdint.h"
#include"ide.h"
#include"dir.h"
#define MAX_OPEN_FILE_CNT 32

struct file{
    uint32_t fd_pos;        //操作文件指针偏移
    uint32_t fd_flag;       //文件打开权限
    struct inode* fd_inode; //文件inode
};

enum std_fd{
    stdin_no,  //标准输入
    stdout_no, //标准输出
    stderr_no  //标准错误输出
};

/*位图类型*/
enum bitmap_type{
    INODE_BITMAP,  
    BLOCK_BITMAP
};

extern struct file file_table[MAX_OPEN_FILE_CNT];

/*从文件表中获取空闲位并返回下表*/  
extern int32_t alloc_file_table(void);
/*向PCB中文件描述符表找位置安装文件表下表*/
extern int32_t install_pcb_fd(int32_t global_fd_idx);
/*在inode_bitmap中申请inode*/
extern int32_t alloc_inode_bitmap(struct partition*part);
/*在block_bitmap中申请block*/
extern int32_t alloc_block_bitmap(struct partition*part);
/*同步bitmap数据到硬盘*/
extern void sync_bitmap(enum bitmap_type bt,struct partition*part,uint32_t bit_index);
/*创建文件*/
extern int createFile(struct dir* parent_dir,char*file_name,uint32_t flags);

#endif
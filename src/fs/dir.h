#ifndef __DIR_H__
#define __DIR_H__

#define MAX_FILE_NAME_LEN 16

#include"stdint.h"
#include"fs.h"
#include"dir.h"
#include"inode.h"
#include"ide.h"
#include"file.h"
#include"fs.h"
struct dir{
    struct inode* inode;  //指向打开的inode
    uint32_t read_entry_cnt;     //暂时未使用
    uint8_t dir_buf[512]; //页目录数据缓冲区
};

/* 目录的inode指向的数据段*/
struct dir_entry{
    char name[MAX_FILE_NAME_LEN]; //名称
    uint32_t i_no;                //inode号
    enum file_type  f_type;      //文件类型
};

extern struct dir root_dir;

extern void open_root_dir(struct partition*part);
/*打开inode_no号目录*/
extern struct dir* open_dir(struct partition*part,uint32_t inode_no);
/*关闭目录*/
extern void close_dir(struct dir*dir);
/*在内存中设置 目录条目*/
extern void init_dir_entry(struct dir_entry*p_de,char*filename,uint32_t inode_no,enum file_type ft);
/*用名查询目录条目*/
extern bool search_dir_entry_by_name(struct partition*part,struct dir*pdir,const char*name,struct dir_entry*dir_e);
/*用inode_no查询目录条目 ,搜寻到的目录条目存储在dir_e中，由调用者提供*/
extern bool search_dir_entry_by_inode_no(struct partition*part,struct dir*pdir,int inode_no,struct dir_entry*dir_e);
//将dir_entry同步到磁盘
extern bool sync_dir_entry(struct dir* parent_dir,struct dir_entry* d_entry);
//从目录中删除dir_entry
extern bool remove_dir_entry(struct dir*parent_dir,uint32_t inode_no,struct partition*part);

extern struct dir_entry* read_dir_entry(struct dir* parent_dir,struct partition*part);

extern bool remove_empty_dir(struct dir*parent_dir,struct dir*dir,struct partition*part);
#endif
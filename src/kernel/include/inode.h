#ifndef __INODE_H__
#define __INODE_H__
#include"stdint.h"
#include"list.h"
#include"fs.h"

/* 将来会被写入硬盘中 inode表 ， inode表项*/
struct inode{
    uint32_t i_index;               //在inode表中的索引
    uint32_t i_size;                //inode是文件时，i_size为文件大小 ， inode是指该目录下所有目录项大小之和

    uint32_t i_open_cnts;           //文件被打开次数
    uint32_t i_sectors[13];         //12个扇区数据，1个扇区为1级间接块指针

    enum file_types file_type;	    //文件类型
    uint32_t hard_link_cnt; 	    //硬链接数
    enum privilege mode;	    	//权限

    bool write_denyl;               //防止进程同时写入
    struct list_elem inode_node;    //内存中需链接起相应inode
};

#endif
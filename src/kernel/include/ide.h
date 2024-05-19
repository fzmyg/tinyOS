#ifndef __IDE_H__
#define __IDE_H__
#include"stdint.h"
#include"list.h"
#include"bitmap.h"
#include"sync.h"
/*分区结构*/
struct partition{
    uint32_t start_lba;             //分区起始扇区
    uint32_t sector_cnt;            //扇区数
    struct disk*my_disk;            //分区所属的硬盘
    struct list_elem part_node;     //用于队列中的标记
    char name[8];                   //分区名称
    struct super_block*sb;          //本分区的超级块
    struct Bitmap block_bitmap;     //块位图
    struct Bitmap inode_bitmap;     //i节点位图
    struct list open_inodes;        //该分区打开的i节点队列
};
/*硬盘结构*/
struct disk{
    char name[8];                   //硬盘名称
    struct ide_channel* my_channel; //硬盘所属通道
    uint8_t dev_no;                 //硬盘是主盘0还是从盘1
    struct partition prim_parts[4]; //4个主分区
    struct partition logic_parts[8];//无限多个逻辑分区
};
/*ATA通道结构*/
struct ide_channel{
    char name[8];                   //通道名称
    uint16_t port_base;             //通道起始端口号
    uint8_t irq_no;                 //通道所用中断号
    struct lock lock;               //锁
    bool expecting_int;             //等待硬盘中断
    struct semaphore disk_done;     //用于阻塞，唤醒驱动程序
    struct disk devics[2];          //通道上连接俩设备
};

extern void readDisk(struct disk*hd,uint32_t lba_addr,void*buf,uint32_t cnt);

extern void writeDisk(struct disk*hd,uint32_t lba_addr,void*buf,uint32_t cnt);

extern void ide_init(void);

#endif

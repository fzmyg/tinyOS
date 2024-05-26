#ifndef __IDE_H__
#define __IDE_H__
#include"stdint.h"
#include"list.h"
#include"bitmap.h"
#include"sync.h"
/*注释中添加 “*” 表示该成员有意义 ， 没有 “*” 则说明该成员没有实际意义 */
/*分区结构*/
struct partition{
    char name[8];                   //*分区名称
    struct disk*my_disk;            //*分区所属的硬盘
    uint32_t start_lba;             //*分区起始扇区
    uint32_t sector_cnt;            //*扇区数           单个分区最大支持存储 2^32 * 2^9 = 2^41 = 2TB
    struct list_elem part_node;     //*用于队列中的标记
    
    struct super_block*sb;          //*本分区的超级块
    struct Bitmap block_bitmap;     //*块位图 --- 用来表示哪些扇区被使用那些未使用
    struct Bitmap inode_bitmap;     //*i节点位图
    struct list open_inodes;        //*该分区打开的i节点队列
};

/*硬盘结构 */
struct disk{
    char name[8];                   //*硬盘名称
    struct ide_channel* my_channel; //*硬盘所属通道
    uint8_t dev_tag;                //*硬盘是主盘0还是从盘1
    struct partition prim_parts[4]; //*4个主分区
    struct partition logic_parts[8];//*无限多个逻辑分区
};

/*ATA通道结构 主盘和从盘公用一个通道 
 *主盘和从盘共享同一通道的端口基址（port base），但使用不同的寄存器（device寄存器中DEV位）或命令来区分操作是针对主盘还是从盘。
 */
struct ide_channel{
    char name[8];                   //*通道名称
    uint16_t disk_port_base;        //*硬盘操作起始端口号
    struct lock channel_lock;       //*通道锁
    struct semaphore disk_working;  //*用于阻塞，中断程序唤醒继续读或写
    struct disk disks[2];           //*通道上连接俩设备一个主盘一个从盘
    
    uint8_t int_num;                //通道所用中断号
    bool expecting_int;             //标记正在等待硬盘中断
};

extern struct ide_channel channels[2];

extern uint8_t channel_cnt;

extern struct list partition_list;

/*读取磁盘内容到buf*/
extern void readDisk(void*const buf,struct disk*hd,uint32_t lba_addr,uint32_t cnt);

/*写入buf内容到磁盘*/
extern void writeDisk(const void*const buf,struct disk*hd,uint32_t lba_addr,uint32_t cnt);

/*初始化磁盘*/
extern void initIDE(void);

#endif

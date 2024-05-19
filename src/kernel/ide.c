#include"ide.h"
#include"stdint.h"
#include"print.h"
#include"stdio.h"
#include"global.h"
#include"string.h"
#include"debug.h"
#include"io.h"
#include"interrupt.h"
#include"sync.h"
#include"timer.h"
#define reg_data(channel)       (channel->port_base+0)
#define reg_error(channel)      (channel->port_base+1)
#define reg_features(channel)   (channel->port_base+1)
#define reg_sect_cnt(channel)   (channel->port_base+2)
#define reg_lba_l(channel)      (channel->port_base+3)
#define reg_lba_m(channel)      (channel->port_base+4)
#define reg_lba_h(channel)      (channel->port_base+5)
#define reg_dev(channel)        (channel->port_base+6)
#define reg_status(channel)     (channel->port_base+7)
#define reg_command(channel)    (channel->port_base+7)
#define reg_alt_status(channel) (channel->port_base+0x206)
/*status寄存器标属性*/
#define ALT_STAT_BUSY_BIT  0x80 //硬盘正忙
#define ALT_STAT_READY_BIT 0x40 //设备就绪
#define ALT_STAT_DRQ_BIT   0x08 //设备准备好数据
/*device寄存器属性*/
#define DEV_MBS_BIT        0xa0 //固定值
#define DEV_LBA_BIT        0x40 //LBA寻址模式
#define DEV_DEV_BIT        0x10 //主盘为0 从盘为1
/*硬盘操作命令*/
#define CMD_IDENTIFY       0xec //硬盘识别
#define CMD_READ_SECTOR    0x20 //读硬盘
#define CMD_WRITE_SECTOR   0x30 //写硬盘

#define MAX_LBA ((80*1024*1024/512)-1) /*最大扇区数*/

uint8_t channel_cnt;
struct ide_channel channels[2]; /*通道0为主盘通道，通道1为从盘通道*/

static void int_hd_handler(uint32_t irq_no);
void ide_init()
{
    put_str("ide_init start\n");
    uint8_t hd_cnt = *((uint8_t*)(0x475)); //获取硬盘数
    channel_cnt = DIV_ROUND_UP(hd_cnt,2);  //硬盘数/2 向上取整位通道数
    struct ide_channel*channel;              
    int i = 0;
    for(i=0;i<channel_cnt;i++){             //初始化两通道
        memset(channel,0,sizeof(struct ide_channel));
        channel = channels + i; 
        sprintf(channel->name,"ide%d",i);
        switch(i){
            case 0:
                channel->port_base = 0x1f0; // 主盘端口
                channel->irq_no = 0x2e;     // 主盘中断号

                break;
            case 1:
                channel->port_base = 0x170; // 从盘端口
                channel->irq_no = 0x2f;     // 从盘中断号
                break;
        }
        registerIntFunc(channel->irq_no,&int_hd_handler);
        initLock(&channel->lock);
        initSema(&channel->disk_done,0);
    }
    put_str("ide_init done\n");
}

/*选择读写的硬盘*/
static void select_disk(struct disk*hd)
{
    outb(reg_dev(hd->my_channel),DEV_MBS_BIT | DEV_LBA_BIT | ((hd->dev_no==1)?DEV_DEV_BIT:0));
}

/*选择地址和数量*/
static void select_sector(struct disk*hd,uint32_t lba_addr,uint8_t cnt)
{
    lba_addr&=0x0fffffff;
    struct ide_channel*channel=hd->my_channel;
    ASSERT(channel!=NULL);
    outb(reg_sect_cnt(channel),cnt);                     //选择数量 
    outb(reg_lba_l(channel),lba_addr&0x000000ff);        //选择LBA低8位
    outb(reg_lba_m(channel),(lba_addr&0x0000ff00) >> 8); //选择LBA中8位
    outb(reg_lba_h(channel),(lba_addr&0x00ff0000) >> 16);//选择LBA高8位

    outb(reg_dev(channel),(DEV_MBS_BIT | DEV_LBA_BIT | ((hd->dev_no)==1?DEV_DEV_BIT:0)|(lba_addr&0x0f000000)>>24));//选择LBA最高4位
}
/*选择命令*/
static void out_cmd(struct ide_channel*channel,uint8_t cmd)
{
    channel->expecting_int = true;  //期待中断位设置为1
    outb(reg_command(channel),cmd); //向command端口输出命令
}
/*从硬盘读数据*/
static void read_form_disk(struct disk*hd,void*buf,uint8_t sec_cnt)
{
    uint32_t byte_size = sec_cnt==0?256<<9:sec_cnt<<9;
    insw(reg_data(hd->my_channel),buf,byte_size/2);    
}
/*向硬盘写数据*/
static void write2disk(struct disk*hd,void*buf,uint8_t sec_cnt)
{
    uint32_t byte_size = sec_cnt==0?256<<9:sec_cnt<<9;
    outsb(reg_data(hd->my_channel),buf,byte_size/2);   
}
/*判断硬盘是否就绪，如果未就绪就使当前线程睡眠*/
static bool busy_wait(struct disk*hd)
{
    
    uint32_t time_limit = 30 * 1000;                //最多等待30s
    while(time_limit>0){
        if(inb(reg_status(hd->my_channel)&ALT_STAT_BUSY_BIT)==false) //数据准备好了
            return true;
        else
            mtime_sleep(10);
        time_limit -= 10;               //1次时钟滴答
    }
    return false;
}

void readDisk(struct disk* hd,uint32_t lba_addr,void*buf,uint32_t cnt)
{
    ASSERT(lba_addr<MAX_LBA);
    acquireLock(&hd->my_channel->lock); //获取该通道锁
    select_disk(hd);  //选择硬盘，设置device寄存器
    uint32_t reserve_cnt = cnt%256; //一次操作不足256扇区的部分
    uint32_t edge_cnt = cnt/256;    //操作足够256扇区的数量
    uint32_t i = 0 ;
    for(i=0;i<edge_cnt;i++){
        select_sector(hd,lba_addr,(uint8_t)256); //选择扇区数和起始地址
        out_cmd(hd->my_channel,(uint8_t)CMD_READ_SECTOR);//选择硬盘操作
        
        /**********阻塞等待硬盘中断唤醒********/
        semaDown(&hd->my_channel->disk_done);
        /************************************/

        if(busy_wait(hd)==false){ /* 若硬盘操作出错 */
            char error[64];
            sprintf(error,"%s read sector %d failed!!!!\n",hd->name,lba_addr+i*256);
            PANIC(error);
        }
        read_form_disk(hd,(char*)buf+(i*256*512),(uint8_t)256); //向buf中读取数据
    }
    /*操作不足256扇区的部分*/
    select_sector(hd,lba_addr,reserve_cnt);
    out_cmd(hd->my_channel,CMD_READ_SECTOR);
    semaDown(&hd->my_channel->disk_done);
    if(busy_wait(hd)==false){
        char error[64];
        sprintf(error,"%s read sector %d failed!!!!\n",hd->name,lba_addr+i*256);
        PANIC(error);
    }
    read_form_disk(hd,(char*)buf+(i*256*512),reserve_cnt);

    releaseLock(&hd->my_channel->lock);     //释放通道锁
}

void writeDisk(struct disk* hd,uint32_t lba_addr,void*buf,uint32_t cnt)
{
    ASSERT(lba_addr<MAX_LBA);
    acquireLock(&hd->my_channel->lock); //获取该通道锁
    select_disk(hd);  //选择硬盘，设置device寄存器
    uint32_t reserve_cnt = cnt%256; //一次操作不足256扇区的部分
    uint32_t edge_cnt = cnt/256;    //操作足够256扇区的数量
    uint32_t i = 0 ;
    for(i=0;i<edge_cnt;i++){
        select_sector(hd,lba_addr,(uint8_t)256); //选择扇区数和起始地址
        out_cmd(hd->my_channel,(uint8_t)CMD_READ_SECTOR);//选择硬盘操作
        
        /**********阻塞等待硬盘中断唤醒********/
        semaDown(&hd->my_channel->disk_done);
        /************************************/

        if(busy_wait(hd)==false){ /* 若硬盘操作出错 */
            char error[64];
            sprintf(error,"%s read sector %d failed!!!!\n",hd->name,lba_addr+i*256);
            PANIC(error);
        }
        write2disk(hd,(char*)buf+(i*256*512),(uint8_t)256); //向buf中读取数据
    }
    /*操作不足256扇区的部分*/
    select_sector(hd,lba_addr,reserve_cnt);
    out_cmd(hd->my_channel,CMD_READ_SECTOR);
    semaDown(&hd->my_channel->disk_done);
    if(busy_wait(hd)==false){
        char error[64];
        sprintf(error,"%s read sector %d failed!!!!\n",hd->name,lba_addr+i*256);
        PANIC(error);
    }
    write2disk(hd,(char*)buf+(i*256*512),reserve_cnt);

    releaseLock(&hd->my_channel->lock);     //释放通道锁
}

/* 硬盘中断函数 */
static void int_hd_handler(uint32_t irq_no) 
{
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    struct ide_channel* channel = &channels[irq_no-0x2e]; //找到相应中断通道
    ASSERT(channel->irq_no == (uint8_t)irq_no);
    if(channel->expecting_int){         //若相应通道正在等待中断相应则处理，否则不处理
        channel->expecting_int = false; //期待中断位置0
        semaUp(&channel->disk_done);    //唤醒调用硬盘操作的线程   
        inb(reg_status(channel));       //读取status寄存器，让硬盘可继续操作
    }
}
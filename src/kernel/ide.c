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
#include"stdiok.h"
#define reg_data(channel)       (channel->disk_port_base+0)
#define reg_error(channel)      (channel->disk_port_base+1)
#define reg_features(channel)   (channel->disk_port_base+1)
#define reg_sect_cnt(channel)   (channel->disk_port_base+2)
#define reg_lba_l(channel)      (channel->disk_port_base+3)
#define reg_lba_m(channel)      (channel->disk_port_base+4)
#define reg_lba_h(channel)      (channel->disk_port_base+5)
#define reg_dev(channel)        (channel->disk_port_base+6)
#define reg_status(channel)     (channel->disk_port_base+7)
#define reg_command(channel)    (channel->disk_port_base+7)
#define reg_alt_status(channel) (channel->disk_port_base+0x206)
/*status寄存器标属性*/
#define ALT_STAT_BUSY_BIT  0x80 //硬盘正忙
#define ALT_STAT_READY_BIT 0x40 //设备就绪
#define DATA_READY_BIT   0x08 //设备准备好数据
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

int32_t ext_lba_base = 0;           //用于记录总扩展分区的起始lba，初始为0，partition_scan时以此为标记
struct list partition_list;         //分区队列，保存所有分区
/*分区表项*/
struct partition_table_entry{
    uint8_t bootable;        //*可否引导  0x80可引导 0x00不可引导
    uint8_t start_head;      //起始磁头  
    uint8_t start_sect;      //起始扇区
    uint8_t start_chs;       //起始柱面
    uint8_t fs_type;         //*分区类型  0x05拓展分区
    uint8_t end_head;        //结束磁头  
    uint8_t end_sec;         //结束扇区          靠柱面磁头扇区最多可引导8GB磁盘空间
    uint8_t end_chs;         //结束柱面
    uint32_t start_lba;      //*起始LBA地址       靠LBA最多可引导2TB磁盘空间
    uint32_t sec_cnt;        //*总扇区数目
};

/*MBR EBR 共512字节大小 */
struct boot_sector{
    uint8_t other[446];                                 //引导代码段
    struct partition_table_entry partition_table[4];    //分区表
    uint16_t magic_number;                              //0x55,0xaa
}__attribute__((packed));


/*选择读写的硬盘*/
static void select_disk(struct disk*hd)
{
    outb(reg_dev(hd->my_channel),DEV_MBS_BIT | DEV_LBA_BIT | ((hd->dev_tag==1)?DEV_DEV_BIT:0)); //向dev寄存器发送消息
}

/*选择地址和数量*/
static void select_sector(struct disk*hd,uint32_t lba_addr,uint8_t cnt)
{
    lba_addr&=0x0fffffff;
    struct ide_channel*channel=hd->my_channel;
    ASSERT(channel!=NULL);
    outb(reg_sect_cnt(channel),cnt);                     //选择数量  当cnt为0时，规定操作256个扇区
    outb(reg_lba_l(channel),lba_addr&0x000000ff);        //选择LBA低8位
    outb(reg_lba_m(channel),(lba_addr&0x0000ff00) >> 8); //选择LBA中8位
    outb(reg_lba_h(channel),(lba_addr&0x00ff0000) >> 16);//选择LBA高8位

    outb(reg_dev(channel),(DEV_MBS_BIT | DEV_LBA_BIT | ((hd->dev_tag)==1?DEV_DEV_BIT:0)|(lba_addr&0x0f000000)>>24));//选择LBA最高4位
}

/*选择命令*/
static void out_cmd(struct disk* hd,uint8_t cmd)
{
    struct ide_channel* channel = hd->my_channel;
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
    outsw(reg_data(hd->my_channel),buf,byte_size/2);   
}

/*判断硬盘数据是否就绪，如果未就绪就使当前线程睡眠*/
static bool waitDiskData(struct disk*hd)
{
    
    uint32_t time_limit = 30 * 1000;                //最多等待30s
    while(time_limit>0){
        uint8_t status = inb(reg_status(hd->my_channel));
        if((status&DATA_READY_BIT)!=false) //准备好了
            return true;
        else
            mtime_sleep(10);
        time_limit -= 10;               //1次时钟滴答
    }
    return false;
}

/*从硬盘中读取数据到buf*/
void readDisk(void*const buf,struct disk* hd,uint32_t lba_addr,uint32_t cnt)
{
    ASSERT(lba_addr<MAX_LBA);
    acquireLock(&hd->my_channel->channel_lock); //获取该通道锁
    select_disk(hd);  //选择硬盘，设置device寄存器
    uint32_t reserve_cnt = cnt%256; //一次操作不足256扇区的部分
    uint32_t full_batches_cnt = cnt/256;    //操作足够256扇区的数量
    uint32_t i = 0 ;
    for(i=0;i<full_batches_cnt;i++){
        select_sector(hd,lba_addr,(uint8_t)256); //选择扇区数和起始地址
        out_cmd(hd,(uint8_t)CMD_READ_SECTOR);//选择硬盘操作
        
        
        /**********阻塞等待硬盘中断唤醒********/
        semaDown(&hd->my_channel->disk_working);
        /************************************/
        if(waitDiskData(hd)==false){ /* 若硬盘操作出错 */
            char error[64];
            sprintf(error,"%s read sector %d failed!!!!\n",hd->name,lba_addr+i*256);
            PANIC(error);
        }
        read_form_disk(hd,(char*)buf+(i*256*512),(uint8_t)256); //向buf中读取数据
    }
    /*操作不足256扇区的部分*/
    select_sector(hd,lba_addr,reserve_cnt);
    out_cmd(hd,CMD_READ_SECTOR);
    semaDown(&hd->my_channel->disk_working);
    if(waitDiskData(hd)==false){
        char error[64];
        sprintf(error,"%s read sector %d failed!!!!\n",hd->name,lba_addr+i*256);
        PANIC(error);
    }
    read_form_disk(hd,(char*)buf+(i*256*512),reserve_cnt);
    releaseLock(&hd->my_channel->channel_lock);     //释放通道锁
}

/* 从buf中写入数据到硬盘 */
void writeDisk(const void*const buf,struct disk* hd,uint32_t lba_addr,uint32_t sector_cnt)
{
    ASSERT(lba_addr<MAX_LBA);
    acquireLock(&hd->my_channel->channel_lock); //获取该通道锁
    select_disk(hd);  //选择硬盘，设置device寄存器
    uint32_t reserve_cnt = sector_cnt%256; //一次操作不足256扇区的部分
    uint32_t full_batches_cnt = sector_cnt/256;    //操作足够256扇区的数量
    uint32_t i = 0 ;
    for(i=0;i<full_batches_cnt;i++){
        select_sector(hd,lba_addr,(uint8_t)256); //选择扇区数和起始地址
        out_cmd(hd,(uint8_t)CMD_WRITE_SECTOR);//选择硬盘操作
        
        write2disk(hd,(char*)buf+(i*256*512),(uint8_t)256); //向buf中读取数据

        /***********写入成功则会引发硬盘中断唤醒该进程********/
        semaDown(&hd->my_channel->disk_working);
        /**************************************************/
    }
    /* 操作不足256扇区的部分 */
    select_sector(hd,lba_addr,reserve_cnt);
    out_cmd(hd,CMD_WRITE_SECTOR);
    write2disk(hd,(char*)buf+(i*256*512),reserve_cnt);
    semaDown(&hd->my_channel->disk_working);
    releaseLock(&hd->my_channel->channel_lock);     //释放通道锁
}

/* 硬盘中断函数 */
static void int_hd_handler(uint32_t irq_no) 
{
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    struct ide_channel* channel = &channels[irq_no-0x2e]; //找到相应中断通道
    ASSERT(channel->int_num == (uint8_t)irq_no);
    if(channel->expecting_int){                           //若相应通道正在等待中断相应则处理，否则不处理
        channel->expecting_int = false;                   //期待中断位置0
        semaUp(&channel->disk_working);                      //唤醒调用硬盘操作的线程   
        inb(reg_status(channel));                         //读取status寄存器，让硬盘可继续操作
    }
}

/*读取硬盘信息需特殊处理*/
static void swap_pairs_bytes(const char*dst,char*buf,uint32_t len)
{
    uint8_t idx;
    for(idx = 0;idx<len;idx+=2){
        buf[idx+1]=*dst++;
        buf[idx]=*dst++;
    }
    buf[len]=0;
}

/*输出硬盘信息*/
static void identify_disk(struct disk*hd)
{
    char id_info[512];
    select_disk(hd);
    out_cmd(hd,CMD_IDENTIFY);
    semaDown(&hd->my_channel->disk_working);
    if(waitDiskData(hd)==false){
        char error[64];
        sprintf(error,"%s identify failed!!!\n",hd->name);
        PANIC(error);
    }
    read_form_disk(hd,id_info,1);
    char buf[64];
    uint8_t sn_start/*序列号起始地址*/= 10*2,sn_len=20/*序列号字节长度*/,md_start/*型号起始地址*/=26*2,md_len/**/=40;
    swap_pairs_bytes(id_info+sn_start,buf,sn_len);
    printk("  MODULE: %s\n",buf); /*输出硬盘序列号*/
    swap_pairs_bytes(id_info+md_start,buf,md_len);
    uint32_t sectors = *(uint32_t*)(id_info+60*2);
    printk("  SECTORS: %d\n",sectors);/*输出硬盘块数*/
    printk("  CAPACITY :%dMB\n",sectors*512/1024/1024);/*输出硬盘容量*/
}

/*调整MBR分区表，使最后一项为拓展分区项*/
static void adjustPartitionTable(char* buf)
{
    struct partition_table_entry * ppte = ((struct boot_sector*)buf)->partition_table;
    int i = 0;
    struct partition_table_entry ext_pte;
    for(i = 0;i<4;i++)
    {
        if((ppte+i)->fs_type == 0x05){
            ext_pte = ppte[i];
            int j = 0;
            for(j=i;j<3;j++){
                ppte[j]=ppte[j+1]; //
            }
            ppte[3]=ext_pte;
            break;
        }
    }
}

/* 从磁盘读取分区结构 初始化磁盘分区数据结构 */
static void initPartitions(struct disk* hd)
{
    uint32_t lba_addr = 0;      //相对于硬盘0号扇区的偏移地址
    uint32_t ext_lba_base = 0;  //拓展分区相对于硬盘0号扇区总偏移
    uint32_t ext_lba = 0;       //逻辑分区相对于拓展分区偏移
    uint32_t primary_index = 0; //主分区索引
    uint32_t logic_index = 0;   //逻辑分区索引
    bool first_read_tag = true; //用来标记是否第一次读取硬盘
    int i = 0;  //遍历分区表
    while(1){
        char buf[512];
        bool found_extend = false;     //标记是否有拓展分区
        readDisk(buf,hd,lba_addr,1); 
        if(first_read_tag == true){
            first_read_tag = false ;
            adjustPartitionTable(buf); //调整partitionTable 让最后一项为拓展分区项
        }
        struct partition_table_entry * ppte = ((struct boot_sector*)buf)->partition_table; //获取分区表
        
        for(;i<4;i++){ //第一次遍历4项，其余都遍历2项
            if(ppte->fs_type==0x05)/*为拓展分区项*/
            {
                
                if(ext_lba_base==0){  //为主扩展分区项
                    ext_lba_base = ppte->start_lba; // 初始化总拓展分区相对磁盘的偏移
                }else{      //为子扩展分区项
                    ext_lba = ppte->start_lba;      //更改为下一分区在拓展分区中的起始偏移
                }
                lba_addr = ext_lba_base + ext_lba ; //下一个拓展分区的起始地址 = 总拓展分区偏移 +  相对拓展分区偏移
                found_extend = true;
                break;      //退出2级循环
            }
            if(ppte->fs_type!=0) /*为有效分区*/
            {
                if(ext_lba_base == 0){ //为主分区
                    sprintf(hd->prim_parts[primary_index].name,"%s%d",hd->name,primary_index+1); //初始化主分区名称
                    hd->prim_parts[primary_index].start_lba = ppte->start_lba;                   //初始化LBA起始地址
                    hd->prim_parts[primary_index].sector_cnt = ppte->sec_cnt;                    //初始化扇区数
                    hd->prim_parts[primary_index].my_disk = hd;                                  //填加硬盘
                    list_append(&partition_list,&hd->prim_parts[primary_index].part_node);       //填加分区节点到分区链表

                    primary_index++;  
                }else{ //为子扩展分区
                    sprintf(hd->logic_parts[logic_index].name,"%s%d",hd->name,logic_index+5);           //初始化子拓展分区
                    hd->logic_parts[logic_index].start_lba = ppte->start_lba/*逻辑分区内偏移*/ + ext_lba_base /*总拓展分区相对磁盘偏移*/ + ext_lba/*逻辑分区相对拓展分区偏移*/;  //初始化LBA起始地址
                    hd->logic_parts[logic_index].sector_cnt = ppte->sec_cnt;                            //初始化扇区数
                    hd->logic_parts[logic_index].my_disk = hd;                                          //添加硬盘
                    list_append(&partition_list,&hd->logic_parts[logic_index].part_node);               //添加分区节点到分区链表

                    logic_index++;
                }
            }
            ppte++;
        }
        if(found_extend == false){ 
            break;//链表结束则退出1级循环
        }
        else {
            i=2; //未结束则控制下一次遍历数量
        }
    }
}

/*初始化IDE硬盘 挂载通道到内存*/
void initIDE()
{
    put_str("ide_init start\n");
    uint8_t hd_cnt = *((uint8_t*)(0x475)); //获取硬盘数
    channel_cnt = DIV_ROUND_UP(hd_cnt,2);  //硬盘数/2 向上取整位通道数
    struct ide_channel*channel;              
    int i = 0;
    for(i=0;i<channel_cnt;i++){             //初始化两通道
        channel = channels + i; 
        memset(channel,0,sizeof(struct ide_channel));
        sprintf(channel->name,"ide%d",i);
        switch(i){
            case 0:
                channel->disk_port_base = 0x1f0; // 主盘端口
                channel->int_num = 0x2e;         // 主盘中断号
                break;
            case 1:
                channel->disk_port_base = 0x170; // 从盘端口
                channel->int_num = 0x2f;         // 从盘中断号
                break;
        }
        registerIntFunc(channel->int_num,&int_hd_handler);   //注册硬盘中断函数
        initLock(&channel->channel_lock);         //初始化通道锁
        initSema(&channel->disk_working,0);  //信号量初值设置为0
        //初始化channel中的disks
        int dev_no = 0;
        for(;dev_no<2;dev_no++){ 
            struct disk* disk = &channel->disks[dev_no];
            disk->my_channel = channel;
            disk->dev_tag = dev_no;
            sprintf(disk->name,"sd%c",'a'+dev_no);
            identify_disk(disk);
            if(dev_no!=0){ //若为从盘则扫描盘上分区表
                initPartitions(disk); //挂载磁盘中的分区
            }
        }
    }
    put_str("ide_init done\n");
}

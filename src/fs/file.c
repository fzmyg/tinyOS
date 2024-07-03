#include "file.h"
#include "thread.h"
#include "ide.h"
#include"super_block.h"
#include"fs.h"
#include"string.h"
#include"stdiok.h"
#include"inode.h"
#include"dir.h"
#include"interrupt.h"
#include"debug.h"
#include"math.h"
#define DIV_ROUND_UP(a,b) ((((a)-1)/(b)) + 1)

/*文件描述表*/
struct file file_table[MAX_OPEN_FILE_CNT];

/*从文件表中获取一个空闲位，成功返回文件表下标，失败返回-1*/
int32_t alloc_file_table(void)
{
    int32_t fd_idx = 3;
    for (; fd_idx < MAX_OPEN_FILE_CNT; fd_idx++)
    {
        if (file_table[fd_idx].fd_inode == NULL)
            break;
    }
    if (fd_idx == MAX_OPEN_FILE_CNT)
    {
        printk("exceed max open files\n");
        return -1;
    }
    return fd_idx;
}

/*向进程文件描述符表中安装文件表索引，成功返回下标，失败返回-1*/
int32_t install_pcb_fd(int32_t global_fd_idx)
{
    struct task_struct *pcb = getpcb();
    uint32_t local_fd_idx = 3;
    for (; local_fd_idx < MAX_FILES_OPEN_PER_PROCESS; local_fd_idx++)
    {
        if (pcb->fd_table[local_fd_idx] == -1) //找到空位
        {
            pcb->fd_table[local_fd_idx] = global_fd_idx;//安装
            break; //跳出循环
        }
    }
    if (local_fd_idx == MAX_FILES_OPEN_PER_PROCESS) //未找到空位
    {
        printk("exceed max open files_per_proc\n");
        return -1;
    }
    return local_fd_idx;
}

/*在inode_bitmap中申请inode*/
int32_t alloc_inode_bitmap(struct partition *part)
{
    int32_t bit_index = scanBitmap(&part->inode_bitmap, 1);
    if (bit_index == -1)
        return -1;
    setBitmap(&part->inode_bitmap, bit_index, 1);
    return bit_index;   //返回inode编号
}

/*在block_bitmap中申请block*/
int32_t alloc_block_bitmap(struct partition*part)
{
    int32_t bit_index = scanBitmap(&part->block_bitmap, 1);
    if (bit_index == -1)
        return -1;
    setBitmap(&part->block_bitmap, bit_index,1);
    return part->sb->data_start_lba+bit_index; //返回申请的块的LBA
}

/*同步bitmap数据到硬盘*/
void sync_bitmap(enum bitmap_type bp_type,struct partition*part,uint32_t bit_index)
{
    uint32_t sector_off = bit_index/(SECTOR_SIZE*8); //得到位图相对在位图起点的硬盘偏移
    uint32_t byte_off  = sector_off*SECTOR_SIZE;    //加上位图起始内存地址就是要写入硬盘的1扇区数据的内存地址
    uint32_t sec_lba; uint8_t * bitmap_off;
    switch(bp_type){
        case INODE_BITMAP:
            sec_lba = part->sb->inode_bitmap_lba + sector_off; //获取要写入的扇区地址
            bitmap_off = part->inode_bitmap.pbitmap+byte_off; //获取要写入数据的内存地址
            break;
        case BLOCK_BITMAP:
            sec_lba = part->sb->block_bitmap_lba + sector_off;  //获取要写入的扇区地址
            bitmap_off = part->block_bitmap.pbitmap + byte_off;//获取要写入数据的内存地址
            break;
    }
    writeDisk(bitmap_off,part->my_disk,sec_lba,1); //写入硬盘
}

/*创建文件*/  //1.申请inode号 2.在内存中创建inode 3.设置文件表 4.设置页目录项 5.磁盘同步父目录 6.同步新文件inode 7.同步当前inode_bitmap 8.将inode加入打开的inode链表中 9.获取文件描述符并返回
int createFile(struct dir* parent_dir,char*file_name,uint32_t flags,struct partition*part) 
{
    uint32_t rollval = -1;
    int32_t inode_no = alloc_inode_bitmap(part); //申请inode号
    if(inode_no==-1){
        printk("createFile: alloc_inode_bitmap error\n");
        return -1;
    }
    struct task_struct * pcb = getpcb();
    uint32_t pgdir_vaddr = pcb->pgdir_vaddr;
    pcb->pgdir_vaddr = NULL;
    struct inode* inode = sys_malloc(sizeof(struct inode)); //挂载inode
    pcb->pgdir_vaddr = pgdir_vaddr;
    if(inode==NULL){
        printk("createFile: sys_malloc for inode faild\n");
        rollval = 1;
        goto rollerror;
    }
    init_inode(inode,inode_no,FT_REGULAR,part); //为内存中inode赋值
    int ft_index =  alloc_file_table(); //申请file结构体
    if(ft_index==-1){
        printk("exceed max open files\n");
        rollval = 2;
        goto rollerror;
    }
    file_table[ft_index].fd_inode=inode; //为file结构体赋值
    file_table[ft_index].fd_flag=flags; 
    file_table[ft_index].fd_pos=0;

    struct dir_entry dir_e;
    init_dir_entry(&dir_e,file_name,inode_no,FT_REGULAR,part->name);
    if(sync_dir_entry(parent_dir,&dir_e,part)==false){ //向磁盘同步dir_entry
        printk("createFile: sync_dir_entry error\n");
        rollval = 3;
        goto rollerror;
    }    

    if(sync_inode(inode,part)==false){ //同步inode
        printk("createFile: sync_inode error\n");
        rollval = 4;
        goto rollerror;
    } 
    sync_bitmap(INODE_BITMAP,part,inode_no); //同步inode位图
    list_push(&part->open_inodes,&inode->i_node); //加入打开的inode队列
    inode->i_open_cnts = 1;
    return install_pcb_fd(ft_index); //返回fd
rollerror:
    switch(rollval){
        case 4:
            remove_dir_entry(parent_dir,dir_e.i_no,part);
        case 3:
            memset(file_table+ft_index,0,sizeof(struct file));
        case 2:
            sys_free(inode);
        case 1:
            setBitmap(&part->inode_bitmap,inode_no,0);
            break;
    }
    return -1;
}

int32_t open_file(uint32_t i_no,uint32_t o_mode,struct partition*part)
{
    int ft_idx = alloc_file_table();
    if(ft_idx == -1){
        printk("exceed max open file count\n");
        return -1;
    }
    file_table[ft_idx].fd_inode = open_inode(part,i_no);
    if(file_table[ft_idx].fd_inode==NULL){
        printk("exceed max open inode count\n");
        return -1;
    }
    file_table[ft_idx].fd_pos=0;
    file_table[ft_idx].fd_flag=o_mode;
    bool * write_deny = &file_table[ft_idx].fd_inode->write_deny;
    if(o_mode&O_WRONLY || o_mode&O_RDWR){
        enum int_status stat = closeInt();
        if(*(write_deny)==false){
            *write_deny = true;
            setIntStatus(stat);
        }else{
            setIntStatus(stat);
            printk("file can not be write now,other process is writeing now\n");
            memset(file_table+ft_idx,0,sizeof(struct file));
            return -1;
        }

    }
    return install_pcb_fd(ft_idx);
}

int32_t close_file(struct file*file)
{
    if(file==NULL)  return -1;
    close_inode(file->fd_inode); //将inode移除内存，未同步磁盘
    memset(file,0,sizeof(struct file));
    return 0;
}

//为指定inode申请数据块,返回数据块地址
int32_t allocDataBlock(struct partition*part,struct inode*inode)
{
    int32_t ret =  -1;
    ASSERT(part!=NULL && inode!=NULL);
    uint32_t* block_addr_buf = (uint32_t*)sys_malloc(sizeof(uint32_t)*140);
    if(block_addr_buf==NULL){
        printk("sys_malloc for block_addr_buf error\n");
        return -1;
    }
    memcpy(block_addr_buf,inode->i_sectors,12*4);
    if(inode->i_sectors[12]!=0)
        readDisk(block_addr_buf+12,part->my_disk,inode->i_sectors[12],1);
    int32_t block_lba = alloc_block_bitmap(part);
    if(block_lba==-1){
        printk("disk full\n");
        sys_free(block_addr_buf);
        return -1;
    }
    uint32_t i = 0;
    for(;i<140;i++){
        if(block_addr_buf[i]==0){
            if(i<12){
                inode->i_sectors[i]=block_lba;
                if(sync_inode(inode,part)==false){
                    setBitmap(&part->block_bitmap,block_addr_buf[i]-part->sb->data_start_lba,0);
                    inode->i_sectors[i]=0;
                    sys_free(block_addr_buf);
                    return -1;
                }
                ret = block_lba;
            }else if(i==12){
                int32_t data_block_lba = alloc_block_bitmap(part);
                if(data_block_lba == -1){
                    setBitmap(&part->block_bitmap,block_lba-part->sb->data_start_lba,0);
                    sys_free(block_addr_buf);
                    return -1;
                }
                inode->i_sectors[i]=block_lba;
                if(sync_inode(inode,part)==false){
                    setBitmap(&part->block_bitmap,block_lba-part->sb->data_start_lba,0);
                    inode->i_sectors[i]=0;
                    sys_free(block_addr_buf);
                    return -1;
                }
                block_addr_buf[i]=data_block_lba;
                writeDisk(block_addr_buf+12,part->my_disk,inode->i_sectors[12],1);
                sync_bitmap(BLOCK_BITMAP,part,data_block_lba);
                ret = data_block_lba;
            }else{
                block_addr_buf[i]=block_lba;
                writeDisk(block_addr_buf+12,part->my_disk,inode->i_sectors[12],1);
                ret = block_lba;
            }
            break;
        }
    }
    if(i!=140) sync_bitmap(BLOCK_BITMAP,part,block_lba);
    sys_free(block_addr_buf);
    return ret;
}


//
/*写入数据有两种模式: 1.覆盖写入 2.追加写入 其中覆盖写入保存文件原始数据块但文件大小需重新分配 追加写入按文件大小为偏移，不使用fd_pos*/
int32_t writeFile(struct file*file,const char*buf,uint32_t count)  
{

    if(file->fd_inode->i_size+count > 140*SECTOR_SIZE){ 
        printk("exceed max file size\n");
        return -1;
    }
    int32_t cnt = (int32_t)count;
    uint32_t blocks_index = 0;
    uint32_t blocks_off = 0;
    char* io_buf = (char*)sys_malloc(SECTOR_SIZE);
    if(io_buf==NULL){
        return -1;
    }
    uint32_t * addr_buf = sys_malloc(sizeof(uint32_t)*140);
    if(addr_buf==NULL){
        sys_free(io_buf);
        return -1;
    }
    memcpy(addr_buf,file->fd_inode->i_sectors,sizeof(uint32_t)*12);
    if(file->fd_inode->i_sectors[12])
        readDisk(addr_buf+12,file->fd_inode->part->my_disk,file->fd_inode->i_sectors[12],1);

    
    if(file->fd_flag & O_APPEND)  //追加写入 将pos指针改为文件大小
        file->fd_pos = file->fd_inode->i_size;
    else                          //覆盖写入 将文件大小置为0
        file->fd_inode->i_size = 0;
    uint32_t write_size = 0;
    uint32_t old_pos = file->fd_pos;
    while(cnt>0){
        blocks_index =  file->fd_pos / SECTOR_SIZE;
        blocks_off   =  file->fd_pos % SECTOR_SIZE; 
        write_size = 0;
        if(addr_buf[blocks_index]!=0){
            if(blocks_off){
                write_size = (int32_t)(SECTOR_SIZE - blocks_off)<cnt?SECTOR_SIZE-blocks_off:(uint32_t)cnt;
                readDisk(io_buf,file->fd_inode->part->my_disk,addr_buf[blocks_index],1);
            }else{
                write_size = cnt>SECTOR_SIZE?SECTOR_SIZE:cnt;
                readDisk(io_buf,file->fd_inode->part->my_disk,addr_buf[blocks_index],1);
            }   
        }else{
            int32_t data_lba = allocDataBlock(file->fd_inode->part,file->fd_inode);
            if(data_lba == -1){
                sys_free(io_buf);
                sys_free(addr_buf);
                sync_inode(file->fd_inode,file->fd_inode->part);
                return (int32_t)count - cnt;
            }
            addr_buf[blocks_index]=data_lba;
            write_size = cnt>SECTOR_SIZE?SECTOR_SIZE:cnt;
        }
        
        memcpy(io_buf+blocks_off,buf+count-cnt,write_size);
        file->fd_pos += write_size;
        cnt -= write_size;
        writeDisk(io_buf,file->fd_inode->part->my_disk,addr_buf[blocks_index],1);
    }
    if(file->fd_flag & O_APPEND){
            file->fd_inode->i_size = file->fd_pos;
    }else{
        file->fd_inode->i_size = MAX(file->fd_inode->i_size, old_pos + count);
    }
    /*可能会失效*/
    sync_inode(file->fd_inode,file->fd_inode->part);
    sys_free(io_buf);
    sys_free(addr_buf);
    return (int32_t)count;
}


int32_t readFile(struct file*file,char*buf,uint32_t count)
{
    if(file->fd_pos + count > file->fd_inode->i_size) count = file->fd_inode->i_size - file->fd_pos;
    if(count == 0 ) return -1;
    char * io_buf = (char*)sys_malloc(SECTOR_SIZE);
    if(io_buf==NULL){
        return -1;
    }
    uint32_t blocks_index =0;
    uint32_t blocks_off   =0;
    int32_t cnt = (int32_t)count;
    uint32_t read_size = 0;
    uint32_t * addr_buf = NULL;
    addr_buf = sys_malloc(140*sizeof(uint32_t));
    if(addr_buf==NULL){
        sys_free(io_buf);
        return -1;
    }
    memcpy(addr_buf,file->fd_inode->i_sectors,12*sizeof(uint32_t));
    if(file->fd_inode->i_sectors[12]!=0)
        readDisk(addr_buf+12,file->fd_inode->part->my_disk,file->fd_inode->i_sectors[12],1);
    /*剩余读取*/
    while(cnt>0){
        blocks_index = file->fd_pos/SECTOR_SIZE;
        blocks_off = file->fd_pos % SECTOR_SIZE;
        if(cnt>SECTOR_SIZE-(int32_t)blocks_off){
            read_size = SECTOR_SIZE-blocks_off;
        }else{
            read_size = cnt;
        }
        if(addr_buf[blocks_index]==0) break;
        readDisk(io_buf,file->fd_inode->part->my_disk,addr_buf[blocks_index],1);
        memcpy(buf+count-cnt,io_buf+blocks_off,read_size);
        file->fd_pos += read_size;
        cnt -= read_size;
    }
    sys_free(io_buf);
    sys_free(addr_buf);
    return (int32_t)count-cnt;
}

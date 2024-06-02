#include "file.h"
#include "thread.h"
#include "ide.h"
#include"super_block.h"
#include"fs.h"
#include"string.h"
#include"stdiok.h"
#include"inode.h"
#include"dir.h"
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
        if (pcb->fd_table[local_fd_idx] != -1) //找到空位
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
    setBitmap(&part->block_bitmap, bit_index, 2);
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



/*创建文件*/  //1.申请inode号 2.在内存中创建inode 3.设置文件表 4.设置页目录项 5.磁盘同步父目录 6.同步当前inode 7.同步当前inode_bitmap 8.将inode加入打开的inode链表中 9.获取文件描述符并返回
/*int create_file(struct dir* parent_dir,char*file_name,uint32_t flags) 
{
    int rollback_step = 0; //用于操作失败回滚各种资源状态
    int32_t inode_no = alloc_inode_bitmap(cur_part); //申请文件inode
    if(inode_no==-1){
        printk("in createFile: alloc_inode_bitmap error\n");
        return -1;
    }
    struct inode* new_file_inode = sys_malloc(sizeof(struct inode));
    if(new_file_inode==NULL){
        printk("in createFile: sys_malloc for new_file_inode error\n");
        rollback_step = 1;
        goto rollback;
    }
    init_inode(new_file_inode,inode_no); //初始化inode

    int fd_index = alloc_file_table(); //从文件表中找出空闲项
    if(fd_index == -1){             //打开文件达到上限
        printk("exceed max open files\n");
        rollback_step = 2;
        goto rollback;
    }
    file_table[fd_index].fd_inode = new_file_inode; 
    file_table[fd_index].fd_flag=flags;
    file_table[fd_index].fd_pos=0;
    struct dir_entry dir_entry;
    init_dir_entry(&dir_entry,file_name,inode_no,FT_REGULAR);//初始化新文件的页目录项
    if(sync_dir_entry(parent_dir,&dir_entry)==false) //若页目录同步失败
    {
        printk("sync dir entry to disk failed\n");
        rollback_step = 3;
        goto rollback;
    }
    
    //sync_inode(parent_dir->inode,cur_part);
    sync_inode(new_file_inode,cur_part);        //将新创建文件inode同步到磁盘inode表中
    sync_bitmap(INODE_BITMAP,cur_part,inode_no);//同步磁盘中inode位图

    list_push(&cur_part->open_inodes,&new_file_inode->i_node);
    new_file_inode->i_open_cnts = 1;
    return install_pcb_fd(fd_index); //获取文件描述符
rollback:
    switch(rollback_step){
        case 3: //同步页目录失败
            memset(&file_table[fd_index],0,sizeof(struct file));
        case 2: //打开文件达到上限
            sys_free(new_file_inode);                   
        case 1: //为inode开辟内存失败
            setBitmap(&cur_part->inode_bitmap,inode_no,0);
            break;
    }
    return -1;
}*/

/*创建文件*/  //1.申请inode号 2.在内存中创建inode 3.设置文件表 4.设置页目录项 5.磁盘同步父目录 6.同步新文件inode 7.同步当前inode_bitmap 8.将inode加入打开的inode链表中 9.获取文件描述符并返回
int createFile(struct dir* parent_dir,char*file_name,uint32_t flags) 
{
    uint32_t rollval = -1;
    int32_t inode_no = alloc_inode_bitmap(cur_part);
    if(inode_no==-1){
        printk("createFile: alloc_inode_bitmap error\n");
        return -1;
    }
    struct inode* inode = sys_malloc(sizeof(struct inode));
    if(inode==NULL){
        printk("createFile: sys_malloc for inode faild\n");
        rollval = 1;
        goto rollerror;
    }
    init_inode(inode,inode_no);
    int ft_index =  alloc_file_table();
    if(ft_index==-1){
        printk("exceed max open files\n");
        rollval = 2;
        goto rollerror;
    }
    file_table[ft_index].fd_inode=inode;
    file_table[ft_index].fd_flag=flags;
    file_table[ft_index].fd_pos=0;

    struct dir_entry dir_e;
    init_dir_entry(&dir_e,file_name,inode_no,FT_REGULAR);
    if(sync_dir_entry(parent_dir,&dir_e)==false){
        printk("createFile: sync_dir_entry error\n");
        rollval = 3;
        goto rollerror;
    }    

    sync_inode(inode,cur_part);
    sync_bitmap(INODE_BITMAP,cur_part,inode_no);
    list_push(&cur_part->open_inodes,&inode->i_node);
    inode->i_open_cnts = 1;
    return install_pcb_fd(ft_index);
rollerror:
    switch(rollval){
        case 3:
            memset(file_table+ft_index,0,sizeof(struct file));
        case 2:
            sys_free(inode);
        case 1:
            setBitmap(&cur_part->inode_bitmap,inode_no,0);
            break;
    }
    return -1;
}
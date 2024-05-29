#include "file.h"
#include "thread.h"
#include "ide.h"
#include"super_block.h"
#include"fs.h"
#include"string.h"
#include"stdiok.h"
/*文件描述表*/
struct file file_table[MAX_OPEN_FILE_CNT];

/*从文件表中获取一个空闲位，成功*/
int32_t get_free_slot_in_global(void)
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

int32_t pcb_fd_instatll(int32_t global_fd_idx)
{
    struct task_struct *pcb = getpcb();
    uint32_t local_fd_idx = 3;
    for (; local_fd_idx < MAX_FILES_OPEN_PER_PROCESS; local_fd_idx++)
    {
        if (pcb->fd_table[local_fd_idx] != NULL)
        {
            pcb->fd_table[local_fd_idx] = global_fd_idx;
            break;
        }
    }
    if (local_fd_idx == MAX_FILES_OPEN_PER_PROCESS)
    {
        printk("exceed max open files_per_proc\n");
        return -1;
    }
    return local_fd_idx;
}
/*在inode_bitmap中申请inode*/
int32_t inode_bitmap_alloc(struct partition *part)
{
    int32_t bit_index = scanBitmap(&part->inode_bitmap, 1);
    if (bit_index == -1)
        return -1;
    setBitmap(&part->inode_bitmap, bit_index, 1);
    return bit_index;
}
/*在block_bitmap中申请block*/
int32_t block_bitmap_alloc(struct partition*part)
{
    int32_t bit_index = scanBitmap(&part->block_bitmap, 1);
    if (bit_index == -1)
        return -1;
    setBitmap(&part->block_bitmap, bit_index, 1);
    return part->sb->data_start_lba+bit_index;
}
/*同步bitmap数据到硬盘*/
void sync_bitmap(enum bitmap_type bt,struct partition*part,uint32_t bit_index)
{
    uint32_t sector_off = bit_index/(SECTOR_SIZE*8); //得到位图相对在位图起点的硬盘偏移
    uint32_t byte_off  = sector_off*SECTOR_SIZE;    //加上位图起始内存地址就是要写入硬盘的1扇区数据的内存地址
    uint32_t sec_lba; uint8_t * bitmap_off;
    switch(bt){
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
#include"fs.h"
#include"stdint.h"
#include"ide.h"
#include"inode.h"
#include"global.h"
#include"super_block.h"
#include"dir.h"
#include"debug.h"
#include"string.h"
#include"stdiok.h"
/*创建文件系统*/
static void format_partition(struct partition*part) 
{
/* ******************************
 * 1.计算元信息
 * ******************************/
    uint32_t part_start_lba = part->start_lba;
    uint32_t part_sect_cnt = part->sector_cnt;
    uint32_t boot_sector_cnt = 1 , super_block_sector_cnt = 1;
    uint32_t inode_bitmap_sector_cnt = DIV_ROUND_UP(MAX_INODE_PER_PARTS , 8 * SECTOR_SIZE);
    uint32_t inode_table_sector_cnt = DIV_ROUND_UP(MAX_INODE_PER_PARTS * sizeof(struct inode) , SECTOR_SIZE);
    uint32_t free_sector_cnt = part->sector_cnt - boot_sector_cnt - super_block_sector_cnt - inode_bitmap_sector_cnt - inode_table_sector_cnt;
    uint32_t block_bitmap_sector_cnt = DIV_ROUND_UP(free_sector_cnt,SECTOR_SIZE*8+1);
    uint32_t data_block_sector_cnt = free_sector_cnt - block_bitmap_sector_cnt;

/* ******************************
 * 2.在内存中创建超级块并填充元信息
 * ******************************/
    struct super_block sb;
    sb.magic_number = 0x20040104;
    sb.sector_cnt = part_sect_cnt;
    sb.inode_cnt = MAX_INODE_PER_PARTS;
    sb.partition_lba_base = part_start_lba;
    sb.block_bitmap_lba = part->start_lba + boot_sector_cnt + super_block_sector_cnt;
    sb.block_bitmap_sector_cnt = block_bitmap_sector_cnt;
    sb.inode_bitmap_lba =  part->start_lba + boot_sector_cnt + super_block_sector_cnt + block_bitmap_sector_cnt;
    sb.inode_bitmap_sector_cnt = inode_bitmap_sector_cnt;
    sb.inode_table_lba = part->start_lba + boot_sector_cnt + super_block_sector_cnt + block_bitmap_sector_cnt + inode_bitmap_sector_cnt;
    sb.inode_table_sector_cnt = inode_table_sector_cnt;
    sb.data_start_lba = part->start_lba + boot_sector_cnt + super_block_sector_cnt + block_bitmap_sector_cnt + inode_bitmap_sector_cnt + inode_table_sector_cnt;
    sb.root_inode_index = 0;
    sb.dir_entry_size =  sizeof(struct dir_entry);
    sb.inode_size = sizeof(struct inode);

/* ******************************
 * 3.将超级块写入磁盘
 * ******************************/
    writeDisk(&sb,part->my_disk,part->start_lba+1,1);
    /*创建元数据缓冲区*/
    uint32_t buf_size = ((sb.block_bitmap_sector_cnt>sb.inode_bitmap_sector_cnt?(sb.block_bitmap_sector_cnt>sb.inode_table_sector_cnt?sb.block_bitmap_sector_cnt:sb.inode_table_sector_cnt):sb.inode_bitmap_sector_cnt))*SECTOR_SIZE;
    char* buf = (char*)sys_malloc(buf_size);
    ASSERT(buf!=NULL);

    struct Bitmap bitmap;
    bitmap.pbitmap = (uint8_t*)buf;
    bitmap.bitmap_byte_len = buf_size;
    initBitmap(&bitmap);
/* ******************************
 * 4.将数据块位图写入磁盘
 * ******************************/
    setBitmap(&bitmap,0,1); //根目录数据块占1扇区，所以将位图第0位置0
    /*将块位图中不存在的位置1*/
    uint32_t odd_bit_index = block_bitmap_sector_cnt*4096 - data_block_sector_cnt - 1; 
    uint32_t edge = block_bitmap_sector_cnt * 4096;
    for(;odd_bit_index < edge ;odd_bit_index++){
        setBitmap(&bitmap,odd_bit_index,1);
    }
    writeDisk(buf,part->my_disk,sb.block_bitmap_lba,sb.block_bitmap_sector_cnt);
    memset(buf,0,buf_size);

/* ******************************
 * 5.将inode位图写入磁盘
 * ******************************/
    setBitmap(&bitmap,0,1);
    writeDisk(buf,part->my_disk,sb.inode_bitmap_lba,sb.inode_bitmap_sector_cnt);
    memset(buf,0,buf_size);

/* ******************************
 * 6.将跟目录inode数据区写入磁盘
 * ******************************/
    struct inode* inode = (struct inode*) buf;
    inode->i_index=0;
    inode->i_size = sizeof(struct dir_entry)*2;
    inode->i_sectors[0] = sb.data_start_lba;
    writeDisk(buf,part->my_disk,sb.inode_table_lba,1);
    memset(buf,0,buf_size);

/* ******************************
 * 7.将跟目录inode数据区写入磁盘
 * ******************************/
    struct dir_entry*dir_ety =(struct dir_entry*)buf;
    memcpy(dir_ety,".",1);
    dir_ety->i_index = 0;
    dir_ety->f_type = FT_DIRECTORY;
    dir_ety++;
    memcpy(dir_ety,".",1);
    dir_ety->i_index = 0;
    dir_ety->f_type = FT_DIRECTORY;

    writeDisk(buf,part->my_disk,sb.data_start_lba,1);
    sys_free((void*)buf);
}  

/*扫描通道上设备的分区，在分区中创建文件系统*/
void initFileSystem()
{
    printk("init_file_system start\n");
    struct super_block* sb = sys_malloc(SECTOR_SIZE);
    ASSERT(sb!=NULL);
    uint32_t i = 0;
    for(;i<channel_cnt;i++) //中断通道
    {
        uint32_t dev_index = 0;
        for(dev_index=0;dev_index<2;dev_index++) //设备
        {
            if(dev_index==0)
                continue;
            uint32_t part_index = 0;
            for(;part_index<12;part_index++){ //分区
                struct partition * part = NULL;
                uint32_t index = 0;
                if(part_index<4){
                    part = channels[i].disks[dev_index].prim_parts;
                    index = part_index;
                }else{
                    part = channels[i].disks[dev_index].logic_parts;
                    index = part_index - 4;
                }
                if(part[index].start_lba == 0) //分区不存在
                    continue;
                readDisk(sb,&channels[i].disks[dev_index],part[index].start_lba + 1,1); //读取硬盘超级块
                if(sb->magic_number != 0x20040104) //文件系统不存在
                    format_partition(part);
            }
        }
    }
    sys_free(sb);
    printk("init file systme done\n");
}
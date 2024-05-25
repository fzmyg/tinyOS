#ifndef __SUPER_BLOCK_H__
#define __SUPER_BLCOK_H__
#include"stdint.h"

struct super_block{
    uint32_t magic_number;              //魔数：区分不同的文件系统
    uint32_t sector_cnt;                //分区所占扇区数
    uint32_t inode_cnt;                 //分区inode数
    uint32_t partition_lba_base;        //分区起始地址
    uint32_t block_bitmap_lba;          //块位图起始lba地址
    uint32_t block_bitmap_sector_cnt;   //块位图所占扇区数
    uint32_t inode_bitmap_lba;          //inode位图起始lba地址
    uint32_t inode_bitmap_sector_cnt;   //inode位图所占扇区数
    uint32_t inode_table_lba;           //inode表起始lba地址
    uint32_t inode_table_sector_cnt;    //inode表所占扇区数

    uint32_t data_start_lba;            //数据段起始lba地址
    uint32_t root_inode_index;          //root节点在inode表中的索引
    uint32_t dir_entry_size;            //每个目录表条目大小
    uint32_t inode_size;			   //每个inode大小

    char pad[456];
}__attribute__((packed));

#endif
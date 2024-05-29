#ifndef __DIR_H__
#define __DIR_H__

#define MAX_FILE_NAME_LEN 16

#include"stdint.h"
#include"fs.h"

/* 只在内存中存在 */
struct dir{
    struct inode* inode;
    uint32_t dir_pos;
    uint8_t dir_buf[512]; //页目录数据缓冲区
};

/* 目录的inode指向的数据段*/
struct dir_entry{
    char name[MAX_FILE_NAME_LEN]; //
    uint32_t i_index;
    enum file_types  f_type;
};




#endif
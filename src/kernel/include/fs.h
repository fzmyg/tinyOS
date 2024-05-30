#ifndef __FS_H__
#define __FS_H__
#include"ide.h"
#define MAX_INODE_PER_PARTS 4096    //分区最多创建文件数
#define SECTOR_SIZE 512             //扇区大小
#define BLOCK_SIZE 512              //块字节大小

/*文件类型*/
enum file_types{
    FT_UNKNOW = 0 /*未知文件*/, FT_REGULAR = 1/*普通文件*/ , FT_DIRECTORY=2 /*目录*/
};

enum privilege{
    PRI_EXEC=1,PRI_WRITE=2,PRI_READ=4
};

/*为所有从盘创建文件系统*/
extern void initFileSystem(void);

extern partition* cur_part;

#endif
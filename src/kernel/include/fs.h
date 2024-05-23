#ifndef __FS_H__
#define __FS_H__
#define MAX_FILES_PER_PARTS 4096    //分区最多创建文件数
#define BITS_PER_SECTOR 4096        //512 * 8
#define SECTOR_SIZE 512             //扇区大小
#define BLOCK_SIZE 512              //块字节大小

/*文件类型*/
enum file_types{
    FT_UNKONW = 0 /*未知文件*/, FT_REGULAR = 1/*普通文件*/ , FT_DIRECTORY=2 /*目录*/
};

#endif
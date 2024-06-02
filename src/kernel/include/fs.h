#ifndef __FS_H__
#define __FS_H__
#include"ide.h"
#define MAX_INODE_PER_PARTS 4096    //分区最多创建文件数
#define SECTOR_SIZE 512             //扇区大小
#define BLOCK_SIZE 512              //块字节大小
#define MAX_PATH_LEN 512

/*文件类型*/
enum file_types{
    FT_UNKNOW = 0 /*未知文件*/, FT_REGULAR = 1/*普通文件*/ , FT_DIRECTORY=2 /*目录*/
};

enum privilege{
    PRI_EXEC=0,PRI_WRITE=1,PRI_READ=2
};

enum oflags {
    O_RDONLY = 0, O_WRONLY=1,O_RDWR = 2,O_CREATE=4
};

struct searched_path_record{
    char searched_path[MAX_PATH_LEN];
    struct dir* parent_dir;
    enum file_types f_type;
};

/*为所有从盘创建文件系统*/
extern void initFileSystem(void);

extern uint32_t path_depth_cnt(const char*pathname);

extern uint32_t convertPath(char*path);

extern uint32_t getPathDepth(char* path);

extern int32_t sys_open(const char* path_name,uint32_t o_mode);

extern struct partition* cur_part;

#endif
#ifndef __FILE_H__
#define __FILE_H__
#include"stdint.h"
#include"ide.h"
#define MAX_OPEN_FILE_CNT 32

struct file{
    uint32_t fd_pos;
    uint32_t fd_flag;
    struct inode* fd_inode;
};

enum std_fd{
    stdin_no,
    stdout_no,
    stderr_no
};

enum bitmap_type{
    INODE_BITMAP,
    BLOCK_BITMAP
};

extern int32_t get_free_slot_in_global(void);

extern int32_t pcb_fd_instatll(int32_t global_fd_idx);
/*在inode_bitmap中申请inode*/
extern int32_t inode_bitmap_alloc(struct partition*part);
/*在block_bitmap中申请block*/
extern int32_t block_bitmap_alloc(struct partition*part);
/*同步bitmap数据到硬盘*/
extern void sync_bitmap(enum bitmap_type bt,struct partition*part,uint32_t bit_index);

#endif
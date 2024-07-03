#ifndef __MOUNT_H__
#define __MOUNT_H__

#define MAX_PART_CNT 48
#define MAX_PART_LEN 8
#include"fs.h"
#include"sync.h"
struct mount_map{
    char part_names[MAX_PART_CNT][MAX_PART_LEN];
    char path_names[MAX_PART_CNT][MAX_PATH_LEN];
    char bitmap[6];
    struct lock lock;
};

extern struct mount_map mount_map;

extern struct partition* getPart(const char* file_path);

extern void initMountMap(void);

extern int32_t insertMountMap(const char* mount_point,const char*part_name);
#endif
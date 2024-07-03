#include"mount.h"
#include"ide.h"
#include"string.h"
struct mount_map mount_map;

struct partition* getPart(const char* file_path)
{
    acquireLock(&mount_map.lock);
    uint32_t i = 0,same_cnt=0,max_same_cnt = 0,max_index = -1;
    uint32_t file_path_len = strlen(file_path);
    
    for(;i<MAX_PART_CNT;i++){
        same_cnt = samecnt(file_path,mount_map.path_names[i]);
        if(same_cnt == file_path_len && file_path_len != 1) continue; // *** 若为挂载点则需找到第二长前缀 索引
        if(same_cnt>max_same_cnt){
            max_same_cnt = same_cnt;
            max_index = i;
        }
    }
    struct partition* part = getPartByName(mount_map.part_names[max_index]);
    releaseLock(&mount_map.lock);
    return part;
}

void initMountMap(void)
{
    uint32_t i = 0;
    for(;i<MAX_PART_CNT;i++){
        memset(mount_map.part_names[i],0,MAX_PART_LEN);
        memset(mount_map.path_names[i],0,MAX_PATH_LEN);
    }
    memset(mount_map.bitmap,0,6);
    mount_map.bitmap[0]=1;
    memcpy(mount_map.part_names[0],"sdb1",4);
    memcpy(mount_map.path_names[0],"/",1);
    initLock(&mount_map.lock);
}


int32_t insertMountMap(const char* mount_point,const char*part_name)
{
    acquireLock(&mount_map.lock);
    uint32_t byte_index = 0,bit_off = 0;
    for(;byte_index<6;byte_index++){
        if(mount_map.bitmap[byte_index]==(char)0xff){
            byte_index ++;
            continue;
        }
        for(bit_off=0;bit_off<8;bit_off++){
            if((mount_map.bitmap[byte_index] & 1<<bit_off) == 0){
                mount_map.bitmap[byte_index] |= (1<<bit_off);
                break;
            }
        }
    }
    if(byte_index == 6)
        return -1;
    memcpy(mount_map.part_names[byte_index*8+bit_off],part_name,strlen(part_name));
    memcpy(mount_map.path_names[byte_index*8+bit_off],mount_point,strlen(mount_point));
    releaseLock(&mount_map.lock);
    return 0;
}


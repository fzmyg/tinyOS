#include"dir.h"
#include"inode.h"
#include"string.h"
#include"ide.h"
#include"debug.h"
#include"file.h"
#include"fs.h"
#include"stdiok.h"
struct dir root_dir;
/*打开根目录*/
void open_root_dir(struct partition*part)
{
    root_dir.inode = open_inode(part,0);
    root_dir.dir_pos=0;
    memset(root_dir.dir_buf,0,512);
}
/*打开inode_no号目录*/
struct dir* open_dir(struct partition*part,uint32_t inode_no)
{
    struct dir*dir=(struct dir*)sys_malloc(sizeof(struct dir));
    if(dir==NULL){
       return NULL;
    }
    dir->inode = open_inode(part,inode_no);
    dir->dir_pos=0;
    memset(dir->dir_buf,0,512);
    return dir;
}
/*关闭目录*/
void close_dir(struct dir*dir)
{
    if(dir==&root_dir)  return;
    close_inode(dir->inode);
    sys_free(dir);
}
/*在内存中设置 目录条目*/
void init_dir_entry(struct dir_entry*p_de,char*filename,uint32_t inode_no,enum file_types ft)
{
    memset(p_de,0,sizeof(struct dir_entry));
    uint32_t name_len = strlen(filename);
    ASSERT(name_len<=MAX_FILE_NAME_LEN);
    memcpy(p_de->name,filename,name_len);
    p_de -> f_type = ft;
    p_de -> i_no =inode_no;
}

/*用名查询目录条目 ,搜寻到的目录条目存储在dir_e中，由调用者提供*/
bool search_dir_entry(struct partition*part,struct dir*pdir,const char*name,struct dir_entry*dir_e)
{
    uint32_t block_cnt = 140;
    uint32_t* all_blocks = (uint32_t*)sys_malloc(sizeof(uint32_t)*block_cnt);
    uint32_t i=0;
    for(;i<12;i++){
        all_blocks[i] = pdir->inode->i_sectors[i];
    }
    if(pdir->inode->i_sectors[12]!=0){
        readDisk(all_blocks+12,part->my_disk,pdir->inode->i_sectors[12],1);
    }
    char*buf = (char*)sys_malloc(SECTOR_SIZE);
    struct dir_entry* d_entery = (struct dir_entry*)buf;
    uint32_t max_dir_entery_size = SECTOR_SIZE / part->sb->dir_entry_size;
    for(i=0;i<block_cnt;i++){
        if(all_blocks[i]==0) continue;
        readDisk(buf,part->my_disk,all_blocks[i],1);
        uint32_t j = 0;
        
        for(;j<max_dir_entery_size;j++){
            if(strcmp(d_entery[j].name,name)==0){
                memcpy(dir_e,d_entery,sizeof(struct dir_entry));
                sys_free(all_blocks); sys_free(buf);
                return true;
            }
        }
        memset(buf,0,SECTOR_SIZE);
    }
    sys_free(buf); sys_free(all_blocks);
    return false;
}

//将dir_entry同步到磁盘         
bool sync_dir_entry(struct dir*parent_dir,struct dir_entry* d_entry)
{
    struct inode* dir_inode = parent_dir->inode;        //inode获取数据块
    void* buf = sys_malloc(SECTOR_SIZE);
    if(buf==NULL){ //错误处理
        printk("sync_dir_entry: sys_malloc for buf error\n");
        return false;
    }
    uint32_t dir_entry_cnt_per_sector = SECTOR_SIZE/sizeof(struct dir_entry);
    int32_t block_lba = -1;
    uint8_t block_addr_index = 0;
    uint32_t * all_blocks_addr = sys_malloc(sizeof(uint32_t)*140);              //数据区地址表
    if(all_blocks_addr==NULL){ //错误处理
        printk("sync_dir_entry:sys_malloc for all_blocks_addr error\n");
        sys_free(buf);
        return false;
    }
    memcpy(all_blocks_addr,dir_inode->i_sectors,12*sizeof(uint32_t));           //读取前12个数据区地址
    if(dir_inode->i_sectors[12]!=0){ //若间接块存在
        readDisk(all_blocks_addr+12,cur_part->my_disk,dir_inode->i_sectors[12],1);  //从磁盘读取后128数据区地址 ，读取后all_blocks_addr[12]必不为0
    }
    for(block_addr_index=0;block_addr_index<140;block_addr_index++){ //遍历数据区地址表
        if(all_blocks_addr[block_addr_index]==0){      //有未分配的块，需修改父目录inode 
            block_lba = alloc_block_bitmap(cur_part); //分配块
            if(block_lba==-1){
                printk("alloc block bitmap for sync_dir_entry faild!\n");
                sys_free(buf);
                sys_free(all_blocks_addr);
                return false;
            }
            sync_bitmap(BLOCK_BITMAP,cur_part,block_lba-cur_part->sb->data_start_lba);    //磁盘同步块位图
            if(block_addr_index<12){ //直接块未分配
                dir_inode->i_sectors[block_addr_index]=all_blocks_addr[block_addr_index]=block_lba;   //直接分配数据块地址
            }else if(block_addr_index==12){//第一个间接块未分配
                dir_inode->i_sectors[block_addr_index]=block_lba;    //分配间接块地址
                int32_t data_block_lba = alloc_block_bitmap(cur_part);
                if(data_block_lba==-1){
                    printk("alloc block bitmap for sync_dir_entry faild\n");
                    setBitmap(&cur_part->block_bitmap,block_lba-cur_part->sb->data_start_lba,0);
                    sync_bitmap(BLOCK_BITMAP,cur_part,block_lba-cur_part->sb->data_start_lba);
                    dir_inode->i_sectors[12]=0;
                    sys_free(buf);
                    sys_free(all_blocks_addr);
                    return false;
                }
                sync_bitmap(BLOCK_BITMAP,cur_part,data_block_lba-cur_part->sb->data_start_lba);           //磁盘同步块位图
                all_blocks_addr[block_addr_index]=data_block_lba;
                writeDisk(all_blocks_addr+12,cur_part->my_disk,dir_inode->i_sectors[12],1);//同步间接地址块
            }else{//第二个间接块未分配
                all_blocks_addr[block_addr_index]=block_lba;
                writeDisk(all_blocks_addr+12,cur_part->my_disk,dir_inode->i_sectors[12],1);//同步间接地址块
            }
            memset(buf,0,SECTOR_SIZE);
            memcpy(buf,d_entry,sizeof(struct dir_entry));//将目录项拷贝到buf缓冲区
            writeDisk(buf,cur_part->my_disk,all_blocks_addr[block_addr_index],1); //向磁盘写入目录项
            dir_inode->i_size += sizeof(struct dir_entry);
            sync_inode(dir_inode,cur_part); //向磁盘中同步父目录inode信息
            sys_free(buf);
            sys_free(all_blocks_addr);
            return true;
        }
        /* 若第block_index块已存在,将其读进内存,然后在该块中查找空目录项 */
        readDisk(buf,cur_part->my_disk,all_blocks_addr[block_addr_index],1);
        struct dir_entry* p_de = (struct dir_entry*)buf; //p_de[i]来遍历buf区
        uint32_t i = 0;
        for(;i<dir_entry_cnt_per_sector;i++)
        {
            if(p_de[i].f_type == FT_UNKNOW){
                p_de[i]=*d_entry;
                writeDisk(buf,cur_part->my_disk,all_blocks_addr[block_addr_index],1);//向磁盘写入目录项
                dir_inode->i_size+=sizeof(struct dir_entry);
                sync_inode(dir_inode,cur_part);
                sys_free(buf);
                sys_free(all_blocks_addr);
                return true;
            }
        }
    }
    printk("The directory capacity reaches the upper limit\n");
    sys_free(buf);
    sys_free(all_blocks_addr);
    return false;
}
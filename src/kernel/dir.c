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
    root_dir.read_entry_cnt=0;
    memset(root_dir.dir_buf,0,512);
}
/*打开inode_no号目录*/
struct dir* open_dir(struct partition*part,uint32_t inode_no)
{
    if(inode_no==0)
        return &root_dir;
    struct dir*dir=(struct dir*)sys_malloc(sizeof(struct dir));
    if(dir==NULL){
       return NULL;
    }
    dir->inode = open_inode(part,inode_no);
    dir->read_entry_cnt=0;
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
void init_dir_entry(struct dir_entry*p_de,char*filename,uint32_t inode_no,enum file_type ft)
{
    memset(p_de,0,sizeof(struct dir_entry));
    uint32_t name_len = strlen(filename);
    ASSERT(name_len<=MAX_FILE_NAME_LEN);
    memcpy(p_de->name,filename,name_len);
    p_de -> f_type = ft;
    p_de -> i_no =inode_no;
}

/*用名查询目录条目 ,搜寻到的目录条目存储在dir_e中，由调用者提供*/
bool search_dir_entry_by_name(struct partition*part,struct dir*pdir,const char*name,struct dir_entry*dir_e)
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
    struct dir_entry* d_entry = (struct dir_entry*)buf;
    uint32_t max_dir_entery_cnt = SECTOR_SIZE / part->sb->dir_entry_size;
    for(i=0;i<block_cnt;i++){
        if(all_blocks[i]==0) continue;
        readDisk(buf,part->my_disk,all_blocks[i],1);
        uint32_t j = 0;
        
        for(;j<max_dir_entery_cnt;j++){
            if(strcmp(d_entry[j].name,name)==0){
                memcpy(dir_e,d_entry+j,sizeof(struct dir_entry));
                sys_free(all_blocks); sys_free(buf);
                return true;
            }
        }
        memset(buf,0,SECTOR_SIZE);
    }
    sys_free(buf); sys_free(all_blocks);
    return false;
}

/*用inode_no查询目录条目 ,搜寻到的目录条目存储在dir_e中，由调用者提供*/
bool search_dir_entry_by_inode_no(struct partition*part,struct dir*pdir,int inode_no,struct dir_entry*dir_e)
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
    struct dir_entry* d_entry = (struct dir_entry*)buf;
    uint32_t max_dir_entery_cnt = SECTOR_SIZE / part->sb->dir_entry_size;
    for(i=0;i<block_cnt;i++){
        if(all_blocks[i]==0) continue;
        readDisk(buf,part->my_disk,all_blocks[i],1);
        uint32_t j = 0;
        
        for(;j<max_dir_entery_cnt;j++){
            if(d_entry[j].i_no==(uint32_t)inode_no){
                memcpy(dir_e,d_entry+j,sizeof(struct dir_entry));
                sys_free(all_blocks); sys_free(buf);
                return true;
            }
        }
        memset(buf,0,SECTOR_SIZE);
    }
    sys_free(buf); sys_free(all_blocks);
    return false;
}

//将dir_entry同步到磁盘  创建文件或目录时使用        
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
                dir_inode->i_sectors[block_addr_index]=block_lba;      //分配间接块地址
                int32_t data_block_lba = alloc_block_bitmap(cur_part); //申请一级间接块
                if(data_block_lba==-1){
                    printk("alloc block bitmap for sync_dir_entry faild\n");
                    setBitmap(&cur_part->block_bitmap,block_lba-cur_part->sb->data_start_lba,0); //还原block_bitmap
                    sync_bitmap(BLOCK_BITMAP,cur_part,block_lba-cur_part->sb->data_start_lba);   //同步磁盘
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
//删除dir条目
bool remove_dir_entry(struct dir*parent_dir,uint32_t inode_no,struct partition*part)
{
    struct inode* inode = parent_dir->inode;
    uint32_t roll_no = 0;
    char* io_buf = (char*)sys_malloc(SECTOR_SIZE);
    if(io_buf==NULL){
        goto rollback;
    }    
    uint32_t * addr_buf = (uint32_t*)sys_malloc(sizeof(uint32_t)*140);
    if(addr_buf==NULL){
        roll_no = 1;
        goto rollback;
    }
    memcpy(addr_buf,inode->i_sectors,12*sizeof(uint32_t));
    if(inode->i_sectors[12]!=0) 
        readDisk(addr_buf+12,part->my_disk,inode->i_sectors[12],1);

    uint32_t addr_index = 0;
    uint32_t dir_entry_cnt_per_sector = SECTOR_SIZE/part->sb->dir_entry_size;
    for(;addr_index<140;addr_index++){
        if(addr_buf[addr_index]){
            readDisk(io_buf,part->my_disk,addr_buf[addr_index],1);
            uint32_t i =0;
            uint32_t empty_cnt = 0;
            bool found_tag = false;
                for(i=0;i<dir_entry_cnt_per_sector;i++){
                    struct dir_entry*dir_e = ((struct dir_entry*)io_buf )+ i;
                    if(dir_e->i_no==inode_no){
                        found_tag = true;
                        memset(io_buf+i*part->sb->dir_entry_size,0,part->sb->dir_entry_size);
                        writeDisk(io_buf,part->my_disk,addr_buf[addr_index],1);
                    }
                    if(dir_e->f_type == FT_UNKNOW){
                        empty_cnt ++ ;
                    }
                }
                if(found_tag && empty_cnt == dir_entry_cnt_per_sector - 1){
                    setBitmap(&part->block_bitmap,addr_buf[addr_index]-part->sb->data_start_lba,0);
                    sync_bitmap(BLOCK_BITMAP,part,addr_buf[addr_index]-part->sb->data_start_lba);
                    //回收数据块
                    if(addr_index<12){
                        inode->i_sectors[addr_index]=0;
                        sync_inode(inode,part);
                    }else if(addr_index==12){
                        setBitmap(&part->block_bitmap,inode->i_sectors[12]-part->sb->data_start_lba,0);
                        sync_bitmap(BLOCK_BITMAP,part,inode->i_sectors[12]-part->sb->data_start_lba);
                        inode->i_sectors[addr_index]=0;
                        sync_inode(inode,part);
                    }else{
                        setBitmap(&part->block_bitmap,addr_buf[addr_index]-part->sb->data_start_lba,0);
                        sync_bitmap(BLOCK_BITMAP,part,addr_buf[addr_index]-part->sb->data_start_lba);
                        addr_buf[addr_index]=0;
                        writeDisk(addr_buf,part->my_disk,inode->i_sectors[12],1);
                    }
            }
            if(found_tag)
                break;
        }

    }
    sys_free(addr_buf);
    sys_free(io_buf);
    return true;
    rollback:
    switch (roll_no){
        case 2:
            sys_free(addr_buf);
        case 1:
            sys_free(io_buf);
        case 0:
            return false;
            break;
    }
    return false;
}

struct dir_entry* read_dir_entry(struct dir* parent_dir,struct partition*part)
{
    uint32_t* addr_buf = sys_malloc(sizeof(uint32_t)*140);
    if(addr_buf == NULL){
        return NULL;
    }
    memcpy(addr_buf,parent_dir->inode->i_sectors,12*sizeof(uint32_t));
    if(parent_dir->inode->i_sectors[12]){
        readDisk(addr_buf+12,part->my_disk,parent_dir->inode->i_sectors[12],1);
    }
    int entry_cnt_per_sector = SECTOR_SIZE / part->sb->dir_entry_size;
    int blocks_index = 0; uint32_t read_cnt =0;
    char* io_buf = sys_malloc(SECTOR_SIZE);
    if(io_buf==NULL){
        sys_free(addr_buf);
        return NULL;
    }
    struct dir_entry* dir_e = NULL;
    for(;blocks_index<140;blocks_index++){
        if(addr_buf[blocks_index]==0)
            continue;
        readDisk(io_buf,part->my_disk,addr_buf[blocks_index],1);
        struct dir_entry* d_e = (struct dir_entry*)io_buf;
        int i = 0;
        for(;i<entry_cnt_per_sector;i++){
            if(d_e[i].f_type!=FT_UNKNOW){
                if(read_cnt == parent_dir->read_entry_cnt){
                    dir_e = sys_malloc(sizeof(struct dir_entry));  
                    if(dir_e==NULL){
                        sys_free(addr_buf);sys_free(io_buf);
                        return NULL;
                    }
                    memcpy(dir_e,d_e+i,sizeof(struct dir_entry));
                    parent_dir->read_entry_cnt ++;
                    sys_free(addr_buf);sys_free(io_buf);
                    return dir_e;
                }
                read_cnt++;
            }
        }
    }
    sys_free(addr_buf);sys_free(io_buf);
    return NULL;
}


bool remove_empty_dir(struct dir*parent_dir,struct dir*dir,struct partition*part)
{

    struct dir_entry dir_entry;
    search_dir_entry_by_inode_no(part,parent_dir,dir->inode->i_no,&dir_entry);
    if(remove_dir_entry(parent_dir,dir->inode->i_no,part)==false){
        return false;
    }

    if(remove_inode(dir->inode->i_no,part)==false){
        sync_dir_entry(parent_dir,&dir_entry);
        return false;
    }

    return true;
}
#include"fs.h"
#include"stdint.h"
#include"ide.h"
#include"inode.h"
#include"global.h"
#include"super_block.h"
#include"dir.h"
#include"debug.h"
#include"string.h"
#include"stdiok.h"

struct partition* cur_part;

/*创建文件系统*/
static void format_partition(struct partition*part) 
{
/* ******************************
 * 1.计算元信息
 * ******************************/
    uint32_t part_start_lba = part->start_lba;
    uint32_t part_sect_cnt = part->sector_cnt;
    uint32_t boot_sector_cnt = 1 , super_block_sector_cnt = 1;
    uint32_t inode_bitmap_sector_cnt = DIV_ROUND_UP(MAX_INODE_PER_PARTS , 8 * SECTOR_SIZE);
    uint32_t inode_table_sector_cnt = DIV_ROUND_UP(MAX_INODE_PER_PARTS * sizeof(struct inode) , SECTOR_SIZE);
    uint32_t free_sector_cnt = part->sector_cnt - boot_sector_cnt - super_block_sector_cnt - inode_bitmap_sector_cnt - inode_table_sector_cnt;
    uint32_t block_bitmap_sector_cnt = DIV_ROUND_UP(free_sector_cnt,SECTOR_SIZE*8+1);
    uint32_t data_block_sector_cnt = free_sector_cnt - block_bitmap_sector_cnt;

/* ******************************
 * 2.在内存中创建超级块并填充元信息
 * ******************************/
    struct super_block sb;
    sb.magic_number = 0x20040104;
    sb.sector_cnt = part_sect_cnt;
    sb.inode_cnt = MAX_INODE_PER_PARTS;
    sb.partition_lba_base = part_start_lba;
    sb.block_bitmap_lba = part->start_lba + boot_sector_cnt + super_block_sector_cnt;
    sb.block_bitmap_sector_cnt = block_bitmap_sector_cnt;
    sb.inode_bitmap_lba =  part->start_lba + boot_sector_cnt + super_block_sector_cnt + block_bitmap_sector_cnt;
    sb.inode_bitmap_sector_cnt = inode_bitmap_sector_cnt;
    sb.inode_table_lba = part->start_lba + boot_sector_cnt + super_block_sector_cnt + block_bitmap_sector_cnt + inode_bitmap_sector_cnt;
    sb.inode_table_sector_cnt = inode_table_sector_cnt;
    sb.data_start_lba = part->start_lba + boot_sector_cnt + super_block_sector_cnt + block_bitmap_sector_cnt + inode_bitmap_sector_cnt + inode_table_sector_cnt;
    sb.root_inode_no = 0;
    sb.dir_entry_size =  sizeof(struct dir_entry);
    sb.inode_size = sizeof(struct inode);

/* ******************************
 * 3.将超级块写入磁盘
 * ******************************/
    writeDisk(&sb,part->my_disk,part->start_lba+1,1);
    /*创建元数据缓冲区*/
    uint32_t buf_size = ((sb.block_bitmap_sector_cnt>sb.inode_bitmap_sector_cnt?(sb.block_bitmap_sector_cnt>sb.inode_table_sector_cnt?sb.block_bitmap_sector_cnt:sb.inode_table_sector_cnt):sb.inode_bitmap_sector_cnt))*SECTOR_SIZE;
    char* buf = (char*)sys_malloc(buf_size);
    ASSERT(buf!=NULL);

    struct Bitmap bitmap;
    bitmap.pbitmap = (uint8_t*)buf;
    bitmap.bitmap_byte_len = buf_size;
    initBitmap(&bitmap);
/* ******************************
 * 4.将数据块位图写入磁盘
 * ******************************/
    setBitmap(&bitmap,0,1); //根目录数据块占1扇区，所以将位图第0位置0
    /*将块位图中不存在的位置1*/
    uint32_t odd_bit_index = block_bitmap_sector_cnt*4096 - data_block_sector_cnt - 1; 
    uint32_t edge = block_bitmap_sector_cnt * 4096;
    for(;odd_bit_index < edge ;odd_bit_index++){
        setBitmap(&bitmap,odd_bit_index,1);
    }
    writeDisk(buf,part->my_disk,sb.block_bitmap_lba,sb.block_bitmap_sector_cnt);
    memset(buf,0,buf_size);

/* ******************************
 * 5.将inode位图写入磁盘
 * ******************************/
    setBitmap(&bitmap,0,1);
    writeDisk(buf,part->my_disk,sb.inode_bitmap_lba,sb.inode_bitmap_sector_cnt);
    memset(buf,0,buf_size);

/* ******************************
 * 6.将跟目录inode数据区写入磁盘
 * ******************************/
    struct inode* inode = (struct inode*) buf;
    inode->i_no=0;
    inode->i_size = sizeof(struct dir_entry)*2;
    inode->i_sectors[0] = sb.data_start_lba;
    writeDisk(buf,part->my_disk,sb.inode_table_lba,1);
    memset(buf,0,buf_size);

/* ******************************
 * 7.将跟目录inode数据区写入磁盘
 * ******************************/
    struct dir_entry*dir_ety =(struct dir_entry*)buf;
    memcpy(dir_ety->name,".",1);
    dir_ety->i_no = 0;
    dir_ety->f_type = FT_DIRECTORY;
    dir_ety++;
    memcpy(dir_ety->name,"..",2);
    dir_ety->i_no = 0;
    dir_ety->f_type = FT_DIRECTORY;

    writeDisk(buf,part->my_disk,sb.data_start_lba,1);
    sys_free((void*)buf);
}  

/* 挂载分区上的文件系统元信息 */
static void mountPartion(struct partition*part)
{
    printk("mount partition %s start\n",part->name);
    struct super_block*sb = (struct super_block*)sys_malloc(sizeof(struct super_block));
    ASSERT(sb!=NULL);
    part->sb = sb;
    readDisk(sb,part->my_disk,part->start_lba+1,1); //硬盘中读取超级块
    //读取inode位图和块位图
    uint8_t * buf = (uint8_t*)sys_malloc(SECTOR_SIZE*sb->block_bitmap_sector_cnt);
    ASSERT(buf!=NULL);
    readDisk(buf,part->my_disk,sb->block_bitmap_lba,sb->block_bitmap_sector_cnt);
    part->block_bitmap.pbitmap = buf;
    part->block_bitmap.bitmap_byte_len = SECTOR_SIZE*sb->block_bitmap_sector_cnt;

    buf = sys_malloc(SECTOR_SIZE*sb->inode_bitmap_sector_cnt);
    ASSERT(buf!=NULL);
    readDisk(buf,part->my_disk,sb->inode_bitmap_lba,sb->inode_bitmap_sector_cnt);
    part->inode_bitmap.pbitmap=buf;
    part->inode_bitmap.bitmap_byte_len = SECTOR_SIZE * sb->inode_bitmap_sector_cnt;    
    list_init(&part->open_inodes);
    printk("mount partition %s done\n",part->name);
}


/*扫描通道上设备的分区，在分区中创建文件系统*/
void initFileSystem()
{
    printk("init file system start\n");
    struct super_block* sb = sys_malloc(SECTOR_SIZE);
    ASSERT(sb!=NULL);
    uint32_t i = 0;
    for(;i<channel_cnt;i++) //中断通道
    {
        uint32_t dev_index = 0;
        for(dev_index=0;dev_index<2;dev_index++) //设备
        {
            if(dev_index==0)
                continue;
            uint32_t part_index = 0;
            for(;part_index<12;part_index++){ //分区
                struct partition * part = NULL;
                uint32_t index = 0;
                if(part_index<4){
                    part = channels[i].disks[dev_index].prim_parts;
                    index = part_index;
                }else{
                    part = channels[i].disks[dev_index].logic_parts;
                    index = part_index - 4;
                }
                if(part[index].start_lba == 0) //分区不存在
                    continue;
                readDisk(sb,&channels[i].disks[dev_index],part[index].start_lba + 1,1); //读取硬盘超级块
                if(sb->magic_number != 0x20040104) //文件系统不存在
                    format_partition(part);
                
                if(strcmp(part[index].name,"sdb1")==0){
                    mountPartion(&part[index]);
                    cur_part = &part[part_index];
                    open_root_dir(cur_part);
                }
            }
        }
    }
    sys_free(sb);
    memset(file_table,0,sizeof(struct file)*MAX_OPEN_FILE_CNT);
    printk("init file systme done\n");
}


static char* path_parse(const char* pathname,char* name_store_buf)
{   
    char* p = (char*)pathname;
    while(*p=='/') p++;

    while(*p!='/'&&*p!=0){
        *name_store_buf++=*p++;
    }

    if(p[0]==0)  return NULL;
    return p;    
}

uint32_t path_depth_cnt(const char*pathname)
{
    if(pathname==NULL) 
        return 0;
    char name[MAX_FILE_NAME_LEN];
    char* p = (char*)pathname;
    uint32_t depth = 0;
    p=path_parse(p,name);
    while(name[0]){
        depth++;
        memset(name,0,MAX_FILE_NAME_LEN);
        if(p)
            p=path_parse(p,name);
    }
    return depth;
}
/*将路径转换为标准路径*/
uint32_t convertPath(char*path)
{
    if(path==NULL) return 0;
    uint32_t i = 1,j = 1;
    uint32_t path_len = strlen(path);
    char* really_path = (char*)sys_malloc(path_len+1);
    really_path[0]='/';
    if(really_path==NULL) return 0;
    for(;i<path_len;i++)
    {
        if(path[i]=='/'&&path[i-1]=='/')
            continue;
        really_path[j] = path[i];  
        j++;
    }

    memset(path,0,path_len);
    memcpy(path,really_path,path_len);
    sys_free(really_path);
    return strlen(path);
}

/*获取路径深度*/
uint32_t getPathDepth(char* path)
{
    if(path==NULL)
        return 0;
    uint32_t depth = 0;
    uint32_t len  = convertPath(path);
    uint32_t i = 0;
    for(;i<len;i++){
        if(path[i]=='/')
            depth++;
    }
    return depth;
}

/*靠path_name从根目录起始查找文件，成功返回inode号，失败在p_record中记录错误信息*/
static int search_file(char* path_name,struct searched_path_record* p_record)
{
    if(strcmp(path_name,"/")==0 || strcmp(path_name,"/.")==0 || strcmp(path_name,"/..")==0){ //若查找的是根目录，初始化搜索记录并返回
        p_record->parent_dir = &root_dir;
        p_record->searched_path[0]='\0';
        p_record->f_type = FT_DIRECTORY;
        return 0;
    }
    uint32_t path_len = convertPath(path_name); //将地址转换为标准地址
    if(path_name[0]!='/' || path_len>MAX_PATH_LEN) return -1; //地址格式不正确

    char*sub_path = path_name;          //子路径
    struct dir* parent_dir = & root_dir;
    struct dir_entry dir_entry; //搜索条目记录区
    char name[MAX_FILE_NAME_LEN]={0};//搜索name

    p_record->parent_dir = parent_dir;//初始化 当前搜索目录的父目录
    p_record->f_type = FT_UNKNOW;     //初始化 当前文件类型
    uint32_t parent_inode_no = 0 ;    //初始化已查找的目录i_no
    sub_path=path_parse(path_name,name);//从路径提取待查找项

    while(name[0]){ //若有待查找项
        if(strlen(p_record->searched_path)>MAX_PATH_LEN) return -1; //若搜索路径长度超出定义范围 直接返回，搜索失败
        strcat(p_record->searched_path,"/");
        strcat(p_record->searched_path,name);           //拼接当前待查找项
        if(search_dir_entry(cur_part,parent_dir,name,&dir_entry)){ //若查找出相应的目录项

            if(dir_entry.f_type==FT_DIRECTORY){         //目录项所指文件为目录文件
                parent_inode_no = parent_dir->inode->i_no;
                close_dir(parent_dir);
                parent_dir = open_dir(cur_part,dir_entry.i_no);
                p_record->parent_dir = parent_dir;
                memset(name,0,MAX_FILE_NAME_LEN);
                if(sub_path){
                    sub_path = path_parse(sub_path,name); //下一个循环查找的文件名
                }
                continue;
            }else if(dir_entry.f_type == FT_REGULAR){   //目录项所指文件为普通文件
                p_record->f_type = FT_REGULAR;
                return dir_entry.i_no;
            }
        } else{
            return -1;
        }
    }
    close_dir(p_record->parent_dir);
    p_record->parent_dir = open_dir(cur_part,parent_inode_no);
    p_record->f_type = FT_DIRECTORY;
    return dir_entry.i_no;
}

/*传入待搜索的路径和一个输出参数搜索路径记录块，搜索路径记录块中记录搜索过的路径，父目录inode，搜索的文件类型*/
static int searchFile(char* path,struct searched_path_record*path_record)
{
    if(strcmp(path,"/")==0||strcmp(path,"/.")==0||strcmp(path,"/..")==0){ //若搜索为根目录
        path_record->f_type=FT_DIRECTORY;
        path_record->searched_path[0]='/';
        path_record->parent_dir=&root_dir;
        return 0;
    }
    uint32_t path_len = convertPath(path); //将路径转换为标准路径格式
    if(path[0]!='/'|| path_len>MAX_PATH_LEN) return -1; //若路径不符合标准直接返回
    char *search_name = sys_malloc(MAX_FILE_NAME_LEN); //开辟512字节搜索文件名缓冲区，避免栈溢出从而在栈中开辟 
    if(search_name==NULL){//开辟失败
        return -1;   
    }
    char* sub_path = path_parse(path,search_name);//获取首个要搜索的文件名
    struct dir_entry dir_e; //搜索所用输出参数
    path_record->parent_dir = &root_dir;
    path_record->searched_path[0]='/';
    uint32_t i_no = 0;
    while(search_name[0]!='\0'){
        strcat(path_record->searched_path,search_name);
        if(search_dir_entry(cur_part,path_record->parent_dir,search_name,&dir_e)==true){  //在父目录中成功搜索到
            if(dir_e.f_type==FT_DIRECTORY){ //为目录
                i_no = path_record->parent_dir->inode->i_no;
                close_dir(path_record->parent_dir);//关闭父目录
                path_record->parent_dir = open_dir(cur_part,dir_e.i_no);//将搜索到的目录打开并设置为新的父目录
                path_record->f_type = FT_DIRECTORY;                     
                memset(search_name,0,MAX_FILE_NAME_LEN);    //清除缓冲区
                if(sub_path!=NULL){
                    sub_path = path_parse(sub_path,search_name);//获取下一待搜索文件名
                }
                strcat(path_record->searched_path,"/");
                continue;//开启下一循环
            }else if(dir_e.f_type==FT_REGULAR){
                path_record->f_type=FT_REGULAR; 
                sys_free(search_name);
                return dir_e.i_no;
            }
        }else{
            return -1;
        }
    }
    /*到此说明最后搜查的是目录，需更新记录中的父目录，因为当前记录的父目录是最后的目录，需回溯到上一层*/
    close_dir(path_record->parent_dir);
    path_record->parent_dir = open_dir(cur_part,i_no);
    sys_free(search_name);
    return dir_e.i_no;
}


int32_t sys_open(const char* path_name,uint32_t o_mode)
{
    if(path_name[strlen(path_name)-1]=='/'){
        printk("can not open a directory:%s",path_name);
        return -1;
    }
    if(o_mode>=(O_CREATE+O_RDONLY+O_RDWR+O_WRONLY)){
        printk("the mode about open file error\n");
        return -1;
    }

    int32_t fd = -1;
    struct searched_path_record searched_record;
    memset(&searched_record,0,sizeof(struct searched_path_record));
    uint32_t path_depth = path_depth_cnt((char*)path_name);

    int inode_no = searchFile((char*)path_name,&searched_record);
    uint32_t searched_depth = path_depth_cnt(searched_record.searched_path);
    if(path_depth != searched_depth){
        printk("no such file or directory\n");
        close_dir(searched_record.parent_dir);
        return -1;
    }
    bool found_tag = inode_no==-1?false:true;
    if(searched_record.f_type==FT_DIRECTORY){
        printk("can not open a direcotry with open(),please use opendir()\n");
        close_dir(searched_record.parent_dir);
        return -1;
    }
    if(found_tag == false && (o_mode & O_CREATE)==0){
        printk("in path %s,file %s is not exist\n",searched_record.searched_path,strrchr(path_name,'/')+1);
        close_dir(searched_record.parent_dir);
        return -1;
    }else if(found_tag == true && (o_mode&O_CREATE)!=0){
        printk("file:%S has already exist!\n",path_name);
        close_dir(searched_record.parent_dir);
        return -1;
    }
    switch (o_mode & O_CREATE){
        case O_CREATE:
            fd = createFile(searched_record.parent_dir,strrchr(path_name,'/'),o_mode);
            close_dir(searched_record.parent_dir);
    }
    return fd;
}
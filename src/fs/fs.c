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
#include"interrupt.h"
#include"console.h"
#include"keyboard.h"
#include"mount.h"

/*创建文件系统*/
static void mkfs2disk(struct partition*part) 
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
 * 7.将根目录inode数据区写入磁盘
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


/*为空白分区创建文件系统，挂载sdb1分区的文件系统*/
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
                    mkfs2disk(part);
                mountPartion(&part[index]); //挂载文件系统到内核
            }
        }
    }
    open_root_dir(getPart("/"));
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

static uint32_t getFileName(char*file_name,const char*file_path)
{
    uint32_t len = strlen(file_path);
    uint32_t i = len - 1;
    while(file_path[i--] != '/');
    i+=2;
    memcpy(file_name,file_path+i,len-i);
    return len-i;
}



static int getMeanOfPathPara(char* start,char*end)
{
    int len = end - start ;
    if(len == 2 && start[0]=='.'&&start[1]=='.')
        return 3;
    if(len == 1 && start[0]=='.')
        return 2;
    
    return 1;
}

/*相对路径转换为绝对路径*/
char* convertAbsPath(const char*path)
{
    //将相对路径转换为标准路径 去除连续 //
    char* std_path = sys_malloc(MAX_PATH_LEN);
    if(std_path==NULL) return NULL;
    char* abs_path = sys_malloc(MAX_PATH_LEN);
    if(abs_path == NULL){
        sys_free(std_path);
        return NULL;
    }
    uint32_t abs_path_len = 0;
    uint32_t std_path_len = convertPath(path,std_path);
    //解析相对路径
    int scan_index=1;
    if(std_path[0]=='/'){ //根目录开始
        int i = 0;
        char*p =NULL;
        while((uint32_t)i<std_path_len){
            while(i<(int32_t)std_path_len&&std_path[i]=='/') i++;
            if(i==(int32_t)std_path_len) break;
            for(scan_index=i+1;(uint32_t)scan_index<std_path_len&&std_path[scan_index]!='/';scan_index++);
            int mean = getMeanOfPathPara(std_path+i,std_path+scan_index);
            switch (mean){
                case 1: //为普通路径
                    abs_path[abs_path_len]='/';
                    memcpy(abs_path + abs_path_len + 1,std_path+i,scan_index - i);
                    abs_path_len += (scan_index - i + 1);
                    break;
                case 2: //为 .
                    break;
                case 3: //为 ..
                    p = strrchr(abs_path,'/');
                    if(p!=NULL){
                        abs_path_len -= (strlen(p));
                        memset(p,0,strlen(p));
                    }
                    break;
            }
            i = scan_index;
        }
        if(abs_path_len == 0) {
            abs_path[0] = '/';
            abs_path[1] = 0;
        }
    }else{ //工作目录开始
        //获取当前工作目录
        char* cwd = abs_path;
        int32_t cwd_len = 0;
        cwd_len = sys_getcwd(cwd,MAX_PATH_LEN);
        if(cwd_len==-1){
            sys_free(std_path);
            sys_free(cwd);
            return NULL;
        }
        int i = 0; char* p =NULL;
        scan_index = 0;
        while(i<(int32_t)std_path_len){
            while(i<(int32_t)std_path_len&&std_path[i]=='/') i++;
            if(i==(int32_t)std_path_len) break;
            for(scan_index=i+1;(uint32_t)scan_index<std_path_len&&std_path[scan_index]!='/';scan_index++);
            int mean = getMeanOfPathPara(std_path+i,std_path+scan_index);
            switch (mean){
                case 1: //为普通路径
                    if(cwd[cwd_len-1]!='/'){
                        cwd[cwd_len]='/';
                        memcpy(cwd+cwd_len+1,std_path+i,scan_index-i);
                        cwd_len += (scan_index-i+1);
                    }else{
                        memcpy(cwd+cwd_len,std_path+i,scan_index-i);
                        cwd_len += (scan_index-i);
                    }
                    break;
                case 2: //为 .
                    break;
                case 3: //为 ..
                    p = strrchr(cwd,'/');
                    if(p!=NULL){
                        cwd_len -= (strlen(p));
                        memset(p,0,strlen(p));
                    }
                    break;
            }
            i = scan_index;
        }
    }
    sys_free(std_path);
    if(abs_path[0]==0) abs_path[0]='/';
    return abs_path;
}

/*将路径转换为标准路径*/
uint32_t convertPath(const char*in_path,char*out_path)
{
    if(in_path==NULL || out_path == NULL) return 0;
    uint32_t i = 0,j = 0;
    uint32_t path_len = strlen(in_path);
    for(;i<path_len-1;i++)
    {
        if(in_path[i]=='/'&&in_path[i+1]=='/')
            continue;
        out_path[j] = in_path[i];  
        j++;
    }
    out_path[j] = in_path[i];
    return strlen(out_path);
}

/*获取路径深度*/
uint32_t getPathDepth(const char* path)
{
    if(path==NULL)
        return 0;
    uint32_t depth = 0;
    char* new_path = sys_malloc(MAX_PATH_LEN);
    if(new_path == NULL) return 0;

    uint32_t len  = convertPath(path,new_path);
    uint32_t i = 0;
    for(;i<len;i++){
        if(new_path[i]=='/')
            depth++;
    }
    sys_free(new_path);
    return depth;
}

/*传入待搜索的路径和一个输出参数搜索路径记录块，搜索路径记录块中记录搜索过的路径，父目录inode，搜索的文件类型*/
static int searchFile(const char* path,struct searched_path_record*path_record)
{
    if(strcmp(path,"/")==0||strcmp(path,"/.")==0||strcmp(path,"/..")==0){ //若搜索为根目录
        path_record->f_type=FT_DIRECTORY;
        path_record->searched_path[0]='/';
        path_record->parent_dir=&root_dir;
        return 0;
    }
    char* new_path = sys_malloc(MAX_PATH_LEN);
    if(new_path == NULL) return -1;
    uint32_t path_len = convertPath(path,new_path); //将路径转换为标准路径格式
    if(new_path[0]!='/'|| path_len>MAX_PATH_LEN) return -1; //若路径不符合标准直接返回
    char *search_name = sys_malloc(MAX_FILE_NAME_LEN); //开辟512字节搜索文件名缓冲区，避免栈溢出从而在栈中开辟 
    if(search_name==NULL){//开辟失败
        sys_free(new_path);
        return -1;   
    }
    char* sub_path = path_parse(new_path,search_name);//获取首个要搜索的文件名
    struct dir_entry dir_e; //搜索所用输出参数
    path_record->parent_dir = &root_dir;
    path_record->searched_path[0]='/';
    uint32_t i_no = 0;
    struct partition*parent_part = NULL;
    while(search_name[0]!='\0'){
        strcat(path_record->searched_path,search_name);
        if(search_dir_entry_by_name(path_record->parent_dir->inode->part,path_record->parent_dir,search_name,&dir_e)==true){  //在父目录中成功搜索到
            if(dir_e.f_type==FT_DIRECTORY){ //为目录
                i_no = path_record->parent_dir->inode->i_no;
                parent_part = path_record->parent_dir->inode->part;
                close_dir(path_record->parent_dir);//关闭父目录
                
                path_record->parent_dir = open_dir(getPartByName(dir_e.part_name),dir_e.i_no);//将搜索到的目录打开并设置为新的父目录
                path_record->f_type = FT_DIRECTORY;                     
                memset(search_name,0,MAX_FILE_NAME_LEN);    //清除缓冲区
                if(sub_path!=NULL){
                    sub_path = path_parse(sub_path,search_name);//获取下一待搜索文件名
                }
                strcat(path_record->searched_path,"/");
                continue;//开启下一循环
            }else if(dir_e.f_type==FT_REGULAR){
                path_record->f_type=FT_REGULAR; 
                path_record->part = getPartByName(dir_e.part_name);
                sys_free(search_name);sys_free(new_path);
                return dir_e.i_no;
            }
        }else{
            //if(path_record->parent_dir!=&root_dir)
                //close_dir(path_record->parent_dir);
            sys_free(new_path);
            sys_free(search_name);
            return -1;
        }
    }
    /*到此说明最后搜查的是目录，需更新记录中的父目录，因为当前记录的父目录是最后的目录，需回溯到上一层*/
    close_dir(path_record->parent_dir);
    path_record->parent_dir = open_dir(parent_part,i_no);
    path_record->part = getPartByName(dir_e.part_name);
    sys_free(search_name); sys_free(new_path);
    return dir_e.i_no;
}


int32_t sys_open(const char* path_name,uint32_t o_mode)
{
    if(path_name==NULL) return -1;
    char* abs_path_name = convertAbsPath(path_name);
    if(abs_path_name == NULL) {
        printk("file path format error\n");
        return -1;
    }
    if(abs_path_name[strlen(abs_path_name)-1]=='/'){
        printk("can not open a directory:%s",path_name);
        sys_free(abs_path_name);
        return -1;
    }
    if(o_mode>=(O_CREATE+O_RDONLY+O_RDWR+O_WRONLY)){
        printk("the mode about open file error\n");
        sys_free(abs_path_name);
        return -1;
    }

    int32_t fd = -1;
    struct searched_path_record searched_record;
    memset(&searched_record,0,sizeof(struct searched_path_record));
    uint32_t path_depth = getPathDepth(abs_path_name);

    int inode_no = searchFile(abs_path_name,&searched_record);
    bool found_tag = inode_no==-1?false:true;
    if(found_tag && searched_record.f_type==FT_DIRECTORY){
        printk("can not open a direcotry with open(),please use opendir()\n");
        sys_free(abs_path_name);
        close_dir(searched_record.parent_dir);
        return -1;
    }
    
    uint32_t searched_depth = getPathDepth(searched_record.searched_path);
    if(path_depth != searched_depth){ //查找到的和打开的深度不同
        printk("no such file or directory\n");
        sys_free(abs_path_name);
        close_dir(searched_record.parent_dir);
        return -1;
    }

    if(found_tag == false && (o_mode & O_CREATE)==0){ //未找到也不创建
        printk("in path %s,file %s is not exist\n",searched_record.searched_path,strrchr(abs_path_name,'/')+1);
        close_dir(searched_record.parent_dir);
        sys_free(abs_path_name);
        return -1;
    }else if(found_tag == true && (o_mode&O_CREATE)!=0){//找到了要创建
        printk("file:%S has already exist!\n",path_name);
        close_dir(searched_record.parent_dir);
        sys_free(abs_path_name);
        return -1;
    }
    switch (o_mode & O_CREATE){
        case O_CREATE:
            fd = createFile(searched_record.parent_dir,strrchr(abs_path_name,'/')+1,o_mode,getPart(abs_path_name));
            break;
            //以下为打开已存在文件
        default:
            fd = open_file(inode_no,o_mode,searched_record.part);
            break;
    }
    close_dir(searched_record.parent_dir);
    sys_free(abs_path_name);
    return fd;
}


static int32_t convert_fd2ft_idx(int fd)
{
    int32_t ft_idx = -1;
    if(fd>2){
        struct task_struct * pcb = getpcb();
        ft_idx = pcb->fd_table[fd];
    }
    return ft_idx;
}

int sys_close(int fd)
{
    if(fd == -1) return -1;
    int ft_idx = convert_fd2ft_idx(fd);
    if(ft_idx==-1) return -1;
    close_file(&file_table[ft_idx]);
    getpcb()->fd_table[fd]=-1;
    return 0;
}

int32_t sys_write(int32_t fd,const char* buf ,uint32_t count)
{
    if(fd<0) return -1;
    if(fd == stdout_no || fd == stderr_no){
        if(strlen(buf)==count)
            console_put_str(buf);
        else{
            uint32_t i = 0;
            for(;i<count;i++){
                console_put_char(buf[i]);
            }
        }
        return (int32_t)count;
    }
    int32_t write_byte_cnt = -1;
    struct file*file = &file_table[convert_fd2ft_idx(fd)];
    if((file->fd_flag & O_WRONLY) != 0 || (file->fd_flag & O_RDWR) != 0)
        write_byte_cnt=writeFile(file,buf,count);
    else
        console_put_str("you have no privilige to write this file\n");
    return write_byte_cnt;
}

int32_t sys_read(int32_t fd,char*buf,uint32_t count)
{
    if(count==0) return -1;
    if(fd==stdout_no|| fd==stderr_no || buf==NULL || fd == -1) return -1;
    if(fd == stdin_no){
        uint32_t i = 0;
        while(i<count){
            char ch = ioq_get_char(&kbd_buf);
            buf[i] = ch;
            i++;
        }
        return count;
    }
    int32_t read_byte_cnt = -1;
    struct file* file = &file_table[convert_fd2ft_idx(fd)];
    if(file->fd_flag==O_RDONLY || file->fd_flag == O_RDWR){
        read_byte_cnt = readFile(file,buf,count);
    }else{
        console_put_str("you have no privilige to read this file\n");
    }
    return read_byte_cnt;
}

int32_t sys_lseek(int fd ,int32_t offset,uint32_t whence)
{
    if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END){
        return -1;
    }
    if(fd < 3){
        return -1;
    }
    int32_t ft_idx = convert_fd2ft_idx(fd);
    if(ft_idx==-1){
        return -1;
    }
    struct file* file = &file_table[ft_idx];
    int32_t new_pos = 0;
    switch (whence){
        case SEEK_SET:
            new_pos = (int32_t)offset;
            break;
        case SEEK_CUR:
            new_pos = (int32_t)file->fd_pos + offset;
            break;
        case SEEK_END:
            new_pos = file->fd_inode->i_size + offset;
            break;
    }
    if( new_pos < 0 || (uint32_t)new_pos >= (file->fd_inode->i_size))
        return -1;

    file->fd_pos = (uint32_t)new_pos;
    return new_pos;
}

//删除文件
int32_t sys_unlink(const char* file_path)
{
    if(file_path==NULL) return false;
    char* abs_path = convertAbsPath(file_path);
    if(abs_path==NULL){
        printk("file path format error\n");
        return -1;
    }
    struct searched_path_record record;
    memset(&record,0,sizeof(struct searched_path_record));
    int inode_no = searchFile(abs_path,&record);
    //未找到文件
    if(inode_no==-1){
        printk("can`t remove %s : no such file \n",abs_path);
        close_dir(record.parent_dir);
        sys_free(abs_path);
        return -1;
    }
    //文件类型错误
    if(record.f_type==FT_DIRECTORY){
        printk("can not remove a directory\n");
        close_dir(record.parent_dir);
        sys_free(abs_path);
        return -1;
    }
    //文件正在使用
    int i = 0;
    for(;i<MAX_OPEN_FILE_CNT;i++){
        if(file_table[i].fd_inode!=NULL){
            if(file_table[i].fd_inode->i_no == (uint32_t)inode_no){
                printk("this file is in use,not allow to delete!\n");
                close_dir(record.parent_dir);
                sys_free(abs_path);
                return -1;
            }
        }
    }
    //找到文件并未在使用
    //磁盘中删除目录项
    if(remove_dir_entry(record.parent_dir,inode_no,record.parent_dir->inode->part)==false){
        close_dir(record.parent_dir);
        return -1;
    }
    //删除文件数据项
    if(remove_inode(inode_no,record.part)==false){
        struct dir_entry dir_e;
        dir_e.i_no = inode_no;
        dir_e.f_type = FT_REGULAR;
        getFileName(dir_e.name,abs_path);
        sync_dir_entry(record.parent_dir,&dir_e,record.parent_dir->inode->part);
        close_dir(record.parent_dir);
        sys_free(abs_path);
        return -1;
    }
    sys_free(abs_path);
    return 0;
}
//在分区下创建目录
int32_t sys_mkdir(const char* dir_path)
{
    if(dir_path==NULL) return -1;
    char* abs_path = convertAbsPath(dir_path);
    if(abs_path==NULL){
        printk("directory path format error\n");
        return -1;
    }
    struct searched_path_record record;
    memset(&record,0,sizeof(struct searched_path_record));
    uint32_t roll_no = 0;
    int inode_no = searchFile(abs_path,&record);
    if(inode_no!=-1){
        if(record.f_type==FT_REGULAR){
            printk("file %s has exist\n",strrchr(record.searched_path,'/')+1);
        }else if(record.f_type == FT_DIRECTORY){
            printk("directory %s has exist\n",strrchr(record.searched_path,'/')+1);
        }else{
            printk("searched file error\n");
        }
        sys_free(abs_path);
        close_dir(record.parent_dir);
        return -1;
    }
    struct partition* new_part=getPart(abs_path);
    inode_no = alloc_inode_bitmap(new_part);
    if(inode_no==-1){
        printk("alloc inode bitmap error\n");
        sys_free(abs_path);
        close_dir(record.parent_dir);
        return -1;
    }
    sync_bitmap(INODE_BITMAP,new_part,inode_no);

    struct inode inode;
    init_inode(&inode,inode_no,FT_DIRECTORY,new_part); 
    struct dir_entry dir_entry; 
    
    if(sync_inode(&inode,new_part)==false){
        goto rollback;
    }
    init_dir_entry(&dir_entry,strrchr(record.searched_path,'/')+1,inode_no,FT_DIRECTORY,new_part->name);
    if(sync_dir_entry(record.parent_dir,&dir_entry,record.parent_dir->inode->part)==false){
        roll_no = 1;
        goto rollback;
    }

    struct dir* dir = open_dir(new_part,inode_no);
    if(dir == NULL){
        roll_no = 2;
        goto rollback;
    }
    
    init_dir_entry(&dir_entry,".",inode_no,FT_DIRECTORY,new_part->name);
    if(sync_dir_entry(dir,&dir_entry,new_part)==false){
        roll_no = 3;
        goto rollback;
    }
    init_dir_entry(&dir_entry,"..",record.parent_dir->inode->i_no,FT_DIRECTORY,record.parent_dir->inode->part->name);
    if(sync_dir_entry(dir,&dir_entry,new_part)==false){
        roll_no = 4;
        goto rollback;
    }
    close_dir(record.parent_dir);
    close_dir(dir);
    sys_free(abs_path);
    return inode_no;

rollback:
    switch (roll_no){
        case 4:
            remove_dir_entry(dir,inode_no,new_part);
        case 3:
            close_dir(dir);
        case 2:
            remove_dir_entry(record.parent_dir,inode_no,record.parent_dir->inode->part);
        case 1:
            remove_inode(inode_no,new_part);
        case 0:
            close_dir(record.parent_dir);
            sys_free(abs_path);
            return -1;
            break;
        default:
            break;
    }
    return -1;
}

struct dir* sys_opendir(const char* dir_path)
{
    if(dir_path == NULL) return NULL;
    char* abs_path = convertAbsPath(dir_path);
    if(abs_path == NULL){
        printk("error:waiting some time and try again\n");
        return NULL;
    }
    uint32_t path_len = strlen(abs_path);
    if(abs_path[0]=='/'&&path_len == 1){
        sys_free(abs_path);
        return &root_dir;
    }

    struct searched_path_record record;
    memset(&record,0,sizeof(struct searched_path_record));
    int32_t inode_no = searchFile(abs_path,&record);
    if(inode_no == -1){
        sys_free(abs_path);
        close_dir(record.parent_dir);
        printk("no such directory\n");
        return NULL;
    }
    struct dir* dir = NULL;
    if(record.f_type==FT_DIRECTORY){
        dir = open_dir(record.part,inode_no);
    }else{
        printk("error:this file is not a directory\n");
    }
    sys_free(abs_path);
    close_dir(record.parent_dir);
    return dir;
}

void sys_closedir(struct dir*dir)
{
    if(dir==NULL) return;
    if(dir==&root_dir){
        dir->read_entry_cnt=0;
        return ;
    }
        
    close_dir(dir);
}

struct dir_entry* sys_readdir(struct dir*dir)
{
    return read_dir_entry(dir,dir->inode->part);
}

void sys_rewinddir(struct dir*dir)
{
    dir->read_entry_cnt = 0;
}

int32_t sys_rmdir(const char* dir_path)
{
    if(dir_path == NULL) return false;
    char* abs_path = convertAbsPath(dir_path);
    if(abs_path == NULL){
        printk("directory path format error\n");
        return -1;
    }
    struct searched_path_record record;
    memset(&record,0,sizeof(struct searched_path_record));
    int inode_no = searchFile(abs_path,&record);
    if(inode_no == -1){
        printk("no such directory\n");
        close_dir(record.parent_dir);
        sys_free(abs_path);
        return -1;
    }
    if(record.f_type!=FT_DIRECTORY){
        printk("can not remove a regular file by rmdir\n");
        close_dir(record.parent_dir);
        sys_free(abs_path);
        return -1;
    }
    struct dir* dir = open_dir(record.part,inode_no);
    if(dir == NULL){
        close_dir(record.parent_dir);
        sys_free(abs_path);
        return -1;
    }
    
    if(dir->inode->i_size != 2 * sizeof(struct dir_entry)){
        printk("the directory is not empty,can not remove it.\n");
        close_dir(dir); close_dir(record.parent_dir);
        sys_free(abs_path);
        return -1;
    }

    if(remove_empty_dir(record.parent_dir,dir)==false){
        close_dir(dir); close_dir(record.parent_dir);
        sys_free(abs_path);
        return -1;
    }
    close_dir(dir);
    close_dir(record.parent_dir);
    sys_free(abs_path);
    return 0;
}

//获取指定目录的上级目录inode号
static int get_parent_inode_no(uint32_t inode_no,struct partition*part,struct partition**ppart)
{
    if(inode_no>4095) 
        return -1;
    char* buf = (char*)sys_malloc(SECTOR_SIZE);
    if(buf==NULL) return -1;
    struct inode* inode = open_inode(part,inode_no); //打开当前目录
    if(inode==NULL) {
        sys_free(buf);
        return -1;
    }
    readDisk(buf,part->my_disk,inode->i_sectors[0],1); //读取inode所指向的扇区
    struct dir_entry* d_e = (struct dir_entry*)buf;
    int parent_inode_no = (int)d_e[1].i_no;             //获取第二个目录条目
    *ppart = getPartByName(d_e[1].part_name);
    close_inode(inode);
    sys_free(buf);
    return parent_inode_no;
}
// home/zbc/ ---> /zbc/home 由sys_getcwd调用
static bool reversePath(char*path,uint32_t len)
{
    char* new_path = sys_malloc(len);
    if(new_path==NULL) return false;
    uint32_t new_path_len = 0;
    int i = len - 1 , j = 0;
    for(;i>0;i = j){
        j=i-1;
        while(j>0&&path[j]!='/') j--;
        new_path[new_path_len] = '/';
        if(j==0){
            memcpy(new_path+new_path_len+1,path+j,i-j);
            new_path_len+=(i-j+1);
        }else{
            memcpy(new_path+new_path_len+1,path+j+1,i-j-1);
            new_path_len+=(i-j);
        }
    }
    memcpy(path,new_path,new_path_len);
    sys_free(new_path);
    return true;
}

//获取当前工作目录
int sys_getcwd(char* path_buf,uint32_t size)
{
    if(path_buf == NULL || size == 0) return -1;

    uint32_t path_len = 0;
    struct task_struct * pcb = getpcb();
    uint32_t cur_inode_no =  pcb->cwd_inode_no; //获取当前线程的工作目录
    if(cur_inode_no==0){
        path_len = 1;
        path_buf[0]='/';
        return 1;
    }
    struct partition* ppart = pcb->cur_part;
    while(cur_inode_no!=0){
        
        int parent_inode_no = get_parent_inode_no(cur_inode_no,ppart,&ppart); //获取父目录inode号
        if(parent_inode_no==-1){
            memset(path_buf,0,size);
            return -1;
        }
        struct dir* p_dir = open_dir(ppart,parent_inode_no); //打开父目录
        struct dir_entry dir_entry;
        search_dir_entry_by_inode_no(ppart,p_dir,cur_inode_no,&dir_entry); //搜索相关条目名
        if(dir_entry.f_type!=FT_DIRECTORY){
            close_dir(p_dir);
            memset(path_buf,0,size);
            return -1;
        }
        uint32_t len = strlen(dir_entry.name);
        if(len+1+path_len>size){ //超出缓冲区范围
            close_dir(p_dir);
            memset(path_buf,0,size);
            return -1;
        }
        memcpy(path_buf+path_len,dir_entry.name,len);
        path_buf[path_len+len]='/';
        path_len += (len+1);
        close_dir(p_dir); //关闭打开的父目录
        cur_inode_no = parent_inode_no; //向上递归
    }       
    if(reversePath(path_buf,path_len)==false) //将路径反转
        return -1;

    return path_len;
}

//改变当前线程的工作目录
int sys_chdir(const char* path)
{
    if(path==NULL) return -1;
    char* abs_path = convertAbsPath(path);
    if(abs_path==NULL){
        return -1;
    }
    struct searched_path_record record;
    memset(&record,0,sizeof(struct searched_path_record));
    int inode_no = searchFile(abs_path,&record);
    if(inode_no==-1){ //未找到
        sys_free(abs_path);
        close_dir(record.parent_dir);
        printk("no such directory");
        return -1;
    }

    if(record.f_type!=FT_DIRECTORY){ //找到但不是目录
        printk("file:%s is not a directory\n",record.searched_path);
        sys_free(abs_path);
        close_dir(record.parent_dir);
        return -1;
    }

    struct task_struct * pcb = getpcb();
    pcb->cwd_inode_no = inode_no;
    pcb->cur_part = getPart(abs_path);
    close_dir(record.parent_dir);
    return 0;
}

int sys_stat(const char* file_path,struct file_stat * stat)
{
    char* abs_path = convertAbsPath(file_path);
    if(file_path==NULL){
        return -1;
    }
    struct searched_path_record  record;
    memset(&record,0,sizeof(struct searched_path_record));
    int32_t inode_no = searchFile(abs_path,&record);
    if(inode_no==-1){
        sys_free(abs_path);
        close_dir(record.parent_dir);
        return -1;
    }else if(inode_no==0){
        stat->st_size = root_dir.inode->i_size;
        stat->st_inode_no=0;
        stat->st_ft=FT_DIRECTORY;
        close_dir(record.parent_dir);
        sys_free(abs_path);
        return 0;
    }
    struct inode* inode = open_inode(record.part,inode_no);
    if(inode==NULL){
        sys_free(abs_path);
        close_dir(record.parent_dir);
        return -1;
    }
    stat->st_size = inode->i_size;
    stat->st_ft = inode->file_type;
    close_inode(inode);
    stat->st_inode_no=inode_no;
    sys_free(abs_path);
    close_dir(record.parent_dir);
    return 0;
}
/* 复制文件描述符
 * 参数1 old_fd : 待复制的文件描述符
 */
int sys_dup(const int old_fd)
{
    if(old_fd>=MAX_FILES_OPEN_PER_PROCESS)
        return -1;
    struct task_struct * pcb = getpcb();
    int i = 3;
    for(;i<MAX_FILES_OPEN_PER_PROCESS;i++){
        if(pcb->fd_table[i]==-1){
            pcb->fd_table[i] = pcb->fd_table[old_fd];
            return i;
        }
    }
    return -1;
}

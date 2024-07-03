#include"inode.h"
#include"ide.h"
#include"super_block.h"
#include"stdint.h"
#include"fs.h"
#include"string.h"
#include"interrupt.h"
#include"debug.h"
#include"list.h"
#include"file.h"
#include"stdiok.h"
/*记录inode在硬盘中位置*/
struct inode_position{
    bool two_sector;        /*是否跨扇区*/
    uint32_t sector_lba;    /*起始LBA*/
    uint32_t block_offset;  /*块内偏移*/
};

/*获取inode项在磁盘inode表中详细位置信息*/
static struct inode_position locate_inode(struct partition*part,uint32_t inode_no)
{
    struct inode_position inode_pos;
    ASSERT(inode_no<=MAX_INODE_PER_PARTS);
    struct super_block* sb = part->sb;
    uint32_t inode_size = sb -> inode_size;
    uint32_t byte_offset = inode_size * inode_no;
    uint32_t lba_offset = byte_offset / SECTOR_SIZE;
    uint32_t block_offset = byte_offset % SECTOR_SIZE;
    uint32_t left_size = SECTOR_SIZE - block_offset;
    inode_pos.two_sector = left_size < inode_size ? true:false;
    inode_pos.sector_lba=sb->inode_table_lba+lba_offset;
    inode_pos.block_offset = block_offset;
    return inode_pos;
}
/*同步inode*/
bool sync_inode(const struct inode*inode,struct partition*part)
{
    struct inode_position inode_pos=locate_inode(part,inode->i_no);
    struct inode pure_inode;
    memcpy(&pure_inode,inode,sizeof(struct inode)); //复制inode信息
    pure_inode.i_open_cnts = 0;
    pure_inode.i_node.prev = NULL;  pure_inode.i_node.next = NULL;
    pure_inode.write_deny=0;
    pure_inode.hard_link_cnt = 0;
    pure_inode.part = NULL;
    uint32_t sector_cnt = inode_pos.two_sector?2:1;
    char * buf = sys_malloc(sector_cnt*SECTOR_SIZE);
    if(buf==NULL) {
        printk("file:%s line:%s : error :can`t sync_inode\n",__FILE__,__LINE__);
        return false;
    }
    readDisk(buf,part->my_disk,inode_pos.sector_lba,sector_cnt);
    memcpy(buf+inode_pos.block_offset,&pure_inode,sizeof(struct inode));
    writeDisk(buf,part->my_disk,inode_pos.sector_lba,sector_cnt);
    sys_free(buf);
    return true;
}

/*从硬盘中读取inode项到内存*/
struct inode* open_inode(struct partition* part,uint32_t i_no)
{
    /*在part中open_inodes缓冲区中查找*/
    enum int_status stat = closeInt();
    struct list_elem* pelem = part->open_inodes.head.next;
    while(pelem!=&part->open_inodes.tail){
        struct inode*inode = (struct inode*)elem2entry(struct inode,i_node,pelem);
        if(inode->i_no==i_no){
            inode->i_open_cnts++;
            return inode;
        }
        pelem=pelem->next;
    }
    setIntStatus(stat);
    /*未在打开的inode链表中找到则需在硬盘中找*/
    struct inode_position inode_pos = locate_inode(part,i_no);
    struct task_struct * pcb = getpcb();
    uint32_t addr = pcb->pgdir_vaddr;
    pcb->pgdir_vaddr = NULL;
    struct inode* inode = (struct inode*)sys_malloc(sizeof(struct inode));  //从内核空间开辟内存，达到进程间共享
    if(inode==NULL){
        setIntStatus(stat);
        return NULL;
    }
    pcb -> pgdir_vaddr = addr;
    uint32_t read_cnt = inode_pos.two_sector ? 2 : 1;
    char* buf = (char*)sys_malloc(SECTOR_SIZE*read_cnt);
    if(buf == NULL){
        sys_free(inode);
        setIntStatus(stat);
        return NULL;
    }
    readDisk(buf,part->my_disk,inode_pos.sector_lba,read_cnt);
    memcpy(inode,buf+inode_pos.block_offset,sizeof(struct inode));
    sys_free(buf);
    stat = closeInt();
    list_push(&part->open_inodes,&inode->i_node); //将新打开的inode加入到inode缓冲区队列队首
    setIntStatus(stat);
    inode->i_open_cnts = 1;
    inode->part = part;
    return inode;
}

/*从内存中释放inode*/
void close_inode(struct inode* inode)
{
    enum int_status stat = closeInt();
    ASSERT(inode->i_open_cnts!=0);
    if(--inode->i_open_cnts==0)
    {
        list_remove(&inode->i_node);
        struct task_struct * pcb = getpcb();
        uint32_t addr = pcb->pgdir_vaddr;
        pcb->pgdir_vaddr = NULL;
        sys_free(inode);  //从内核空间释放
        pcb -> pgdir_vaddr = addr;
    }
    setIntStatus(stat);
}

/*初始化inode*/
void init_inode(struct inode*inode,uint32_t i_no,enum file_type file_type,struct partition*part)
{
    memset(inode,0,sizeof(struct inode));
    inode->i_no = i_no;
    inode->file_type =  file_type;
    inode->part = part;
}

//删除inode表 删除数据块位图 删除inode位图  删除父目录中相关项 由
bool remove_inode(uint32_t inode_no,struct partition*part)
{
    if(inode_no>=MAX_INODE_PER_PARTS) return -1;
    
    int roll_no = 0;
    //获取inode所有数据分布地址
    uint32_t* addr_buf = (uint32_t*)sys_malloc(sizeof(uint32_t)*140);
    if(addr_buf == NULL) {
        roll_no = 1;
        goto rollback;
    }
    struct inode * inode = open_inode(part,inode_no);
    if(inode == NULL) {
        roll_no = 2;
        goto rollback;        
    }
    memcpy(addr_buf,inode->i_sectors,12*sizeof(uint32_t));
    if(inode->i_sectors[12]!=0){
        readDisk(addr_buf+12,part->my_disk,inode->i_sectors[12],1);
    }

    //清除block位图
    uint32_t i = 0;
    for(;i<140;i++){
        if(addr_buf[i]!=0){
            setBitmap(&part->block_bitmap,addr_buf[i]-part->sb->data_start_lba,0);
        }
    }
    if(inode->i_sectors[12]!=0) 
        setBitmap(&part->block_bitmap,inode->i_sectors[12]-part->sb->data_start_lba,0);
    //清除inode位图
    setBitmap(&part->inode_bitmap,inode_no,0);
    //同步inode
    //if(sync_inode(inode,part)==false){
    //    roll_no = 3;
    //    goto rollback;
    //}
    //同步位图
    writeDisk(part->block_bitmap.pbitmap,part->my_disk,part->sb->block_bitmap_lba,part->sb->block_bitmap_sector_cnt);
    sync_bitmap(INODE_BITMAP,part,inode_no);
    sys_free(addr_buf);
    close_inode(inode);
    return true;

    rollback:
    switch (roll_no){
        case 3:
            for(i=0;i<140;i++){
                if(addr_buf[i]!=0) 
                    setBitmap(&part->block_bitmap,addr_buf[i]-part->sb->data_start_lba,1);
            }
            if(inode->i_sectors[12]!=0) 
                setBitmap(&part->block_bitmap,inode->i_sectors[12]-part->sb->data_start_lba,1);
        case 2:
            sys_free(addr_buf);
        case 1:
            return false;
            break;
    }
    return false;
}
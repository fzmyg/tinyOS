#include"inode.h"
#include"ide.h"
#include"super_block.h"
#include"stdint.h"
#include"fs.h"
#include"string.h"
#include"interrupt.h"
#include"debug.h"
#include"list.h"
/*记录inode在硬盘中位置*/
struct inode_position{
    bool two_sector;        /*是否跨扇区*/
    uint32_t sector_lba;    /*起始LBA*/
    uint32_t block_offset;  /*块内偏移*/
};

/*获取inode项在磁盘中详细位置信息*/
static struct inode_position locate_inode(struct partition*part,uint32_t inode_no)
{
    struct inode_position inode_pos;
    ASSERT(inode_no<=MAX_INODE_PER_PARTS);
    struct super_block* sb = part->sb;
    uint32_t inode_size = sizeof(struct inode);
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
void sync_inode(const struct inode*inode,struct partition*part)
{
    struct inode_position inode_pos=locate_inode(part,inode->i_no);
    struct inode pure_inode;
    memcpy(&pure_inode,inode,sizeof(struct inode)); //复制inode信息
    pure_inode.i_open_cnts = 0;
    pure_inode.i_node.prev = NULL;  pure_inode.i_node.next = NULL;
    pure_inode.write_denyl=0;
    pure_inode.hard_link_cnt = 0;
    char * buf;
    if(inode_pos.two_sector){
        buf = sys_malloc(2*SECTOR_SIZE);
        ASSERT(buf!=NULL);
        readDisk(buf,part->my_disk,inode_pos.sector_lba,2);
        memcpy(buf+inode_pos.block_offset,&pure_inode,sizeof(struct inode));
        writeDisk(buf,part->my_disk,inode_pos.sector_lba,2);
    }else{
        buf = sys_malloc(1*SECTOR_SIZE);
        ASSERT(buf!=NULL);
        readDisk(buf,part->my_disk,inode_pos.sector_lba,1);
        memcpy(buf+inode_pos.block_offset,&pure_inode,sizeof(struct inode));
        writeDisk(buf,part->my_disk,inode_pos.sector_lba,1);
    }
    sys_free(buf);
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
    pcb -> pgdir_vaddr = addr;
    uint32_t read_cnt = inode_pos.two_sector ? 2 : 1;
    char* buf = (char*)sys_malloc(SECTOR_SIZE*read_cnt);
    ASSERT(buf!=NULL);
    readDisk(buf,part->my_disk,inode_pos.sector_lba,read_cnt);
    memcpy(inode,buf+inode_pos.block_offset,sizeof(struct inode));
    sys_free(buf);
    stat = closeInt();
    list_push(&part->open_inodes,&inode->i_node); //将新打开的inode加入到inode缓冲区队列队首
    setIntStatus(stat);
    inode->i_open_cnts = 1;
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
void init_inode(struct inode*inode,uint32_t i_no)
{
    memset(inode,0,sizeof(struct inode));
    inode->i_no = i_no;
}
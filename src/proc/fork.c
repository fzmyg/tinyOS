#include"fork.h"
#include"thread.h"
#include"string.h"
#include"memory.h"
#include"process.h"
#include"fs.h"
#include"file.h"
#define DIV_ROUND_UP(a,b) (((a)-1)/(b)+1)

static int32_t forkPCB(struct task_struct *child,struct task_struct *parent)
{
    memcpy(child,parent,PG_SIZE);
    child->elapsed_ticks = 0;
    child->pid = createPid();
    if(child->pid == -1) return -1;
    child->parent_pid = parent->pid;
    child->ticks = parent->priority;
    child->status = TASK_READY;
    child->pgdir_vaddr = NULL;
    memset(&child->ready_node,0,sizeof(struct list_elem));
    memset(&child->all_node,0,sizeof(struct list_elem));
    initMemBlockDesc(child->descs);
    return 0;
}

static int32_t copyVaddrBitmap(struct task_struct *child,struct task_struct*parent)
{
    uint32_t vaddr_bitmap_page_cnt = DIV_ROUND_UP(DIV_ROUND_UP((KERNEL_VADDR_START-USER_VADDR_START)/PG_SIZE,8),PG_SIZE);
    void* vaddr = mallocKernelPage(vaddr_bitmap_page_cnt);
    if(vaddr == NULL) return -1;
    child->vaddr_pool.bitmap.pbitmap = (uint8_t*)vaddr;
    child->vaddr_pool.bitmap.bitmap_byte_len = DIV_ROUND_UP((KERNEL_VADDR_START-USER_VADDR_START)/PG_SIZE,8);
    memcpy(child->vaddr_pool.bitmap.pbitmap,parent->vaddr_pool.bitmap.pbitmap,parent->vaddr_pool.bitmap.bitmap_byte_len);
    return 0;
}

static int32_t create_pdt(struct task_struct*pcb)
{
    if(pcb->pgdir_vaddr!=NULL) return -1;
    pcb->pgdir_vaddr = (uint32_t)createPDT(); 
    return 0;
}

static int32_t copyProcessData(struct task_struct * child,struct task_struct*parent,char* buf)
{
    uint32_t byte_index = 0;
    uint32_t bit_index = 0;
    uint32_t bitmap_byte_len = child->vaddr_pool.bitmap.bitmap_byte_len;
    uint8_t * p = parent->vaddr_pool.bitmap.pbitmap;
    for(byte_index=0;byte_index<bitmap_byte_len;byte_index++){
        if(p[byte_index]==0){
            continue;
        }
        for(bit_index = 0;bit_index<8;bit_index++){
            if((p[byte_index]&(BIT_MASK<<bit_index)) == 0)
                continue;
            uint32_t vaddr = parent->vaddr_pool.vm_start + (byte_index*8+bit_index)*PG_SIZE;
            memcpy(buf,(void*)vaddr,PG_SIZE); //父进程数据拷贝到内核缓冲区
            activatePDT(child); //切换子进程页表
            if(malloc1PageByVaddrWithoutVaddrPool(PF_USER,(void*)vaddr)==NULL)
                return -1;
            memcpy((void*)vaddr,buf,PG_SIZE); //拷贝内核缓冲区数据到子进程
            activatePDT(parent);//切换父进程页表
        }
    }
    return 0;
}

extern void int_exit(void);
static void build_child_stack(struct task_struct*child)
{
    struct intr_stack * int_stack = (struct intr_stack*)(((uint32_t)child) + PG_SIZE - sizeof(struct intr_stack));
    int_stack -> eax = 0;
    struct thread_stack * thread_stack = (struct thread_stack*)(((uint32_t)int_stack) - sizeof(struct thread_stack)+12);
    thread_stack->eip = (void(*)(thread_func,void*))&int_exit;
    child->self_kernel_stack = (uint32_t)thread_stack;
}

static void update_file_table(struct task_struct*child)
{
    int i = 3;
    for(;i<MAX_FILES_OPEN_PER_PROCESS;i++){
        if(child->fd_table[i]!=-1){
            file_table[child->fd_table[i]].fd_inode->i_open_cnts++;
        }
    }
}

pid_t sys_fork(void)
{
    struct task_struct * child_pcb = mallocKernelPage(1); //从内核申请子进程PCB内存
    if(child_pcb==NULL) return -1;
    struct task_struct * parent_pcb = getpcb();
    uint32_t roll_val = 0;
    if(forkPCB(child_pcb,parent_pcb)) goto rollback; 	//设置子进程PCB参数
    if(copyVaddrBitmap(child_pcb,parent_pcb)==-1) goto rollback;
    if(create_pdt(child_pcb)==-1) goto rollback;
    char* io_buf = mallocKernelPage(1);  //申请内核拷贝内存
    if(io_buf == NULL) {
        roll_val = 1;
        goto rollback;
    }
    if(copyProcessData(child_pcb,parent_pcb,io_buf)==-1){ //拷贝用户态数据到子进程
        roll_val = 1;
        goto rollback;
    }
    build_child_stack(child_pcb);  //设置内核态子进程栈：等待被调度
    if(find_elem(&thread_all_list,&child_pcb->all_node)==true || find_elem(&thread_ready_list,&child_pcb->ready_node)==true){
        roll_val = 1;
        goto rollback;
    }
    list_append(&thread_all_list,&child_pcb->all_node);
    list_append(&thread_ready_list,&child_pcb->ready_node);
    update_file_table(child_pcb); //更新inode打开状态
    freeKernelPage(io_buf);		//释放缓冲区
    return child_pcb->pid;
rollback:
    switch (roll_val){
        case 1:
            freeKernelPage(io_buf);
        case 0:       
            freeKernelPage(child_pcb);
            break;
    }
    return -1;    
}
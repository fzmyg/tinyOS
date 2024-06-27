#include"wait_exit.h"
#include"list.h"
#include"memory.h"
#include"fs.h"
#include"bitmap.h"
#include"string.h"
#define DIV_ROUND_UP(a,b) (((a)-1)/(b)+1)
//1.回收页表物理地址 2.关闭打开的文件
static int32_t releaseUserResource(struct task_struct*pcb)
{
    //回收用户程序空间
    struct Bitmap*bitmap = &pcb->vaddr_pool.bitmap;
    uint32_t bit_idx = 0;
    uint32_t vaddr = pcb->vaddr_pool.vm_start;
    while(vaddr<0xc0000000){
        bit_idx = (vaddr - pcb->vaddr_pool.vm_start)/PG_SIZE;
        if(bitIsUsed(bitmap,bit_idx)){
            uint32_t*pte_ptr = getPtePtr(vaddr);
            pfree((*pte_ptr) & 0xfffff000);
        }
        vaddr += PG_SIZE;
    }
    //回收页表空间
    uint32_t i = 0;
    uint32_t * pgdir = (uint32_t*)pcb->pgdir_vaddr;
    for(;i<768;i++){
        if((pgdir[i]&1) != 0){
            pfree(pgdir[i]&0xfffff000);
        }
    }
    //关闭打开的文件
    int*fds=pcb->fd_table;
    for(i=3;i<MAX_FILES_OPEN_PER_PROCESS;i++){
        if(fds[i]!=-1){
            sys_close(fds[i]);
        }
    }
    return 0;
}


int32_t sys_exit(int32_t status)
{
    struct task_struct* pcb = getpcb();
    struct task_struct* parent_pcb = NULL;
    struct list_elem* p = thread_all_list.head.next;
    while(p!=&thread_all_list.tail){
        struct task_struct* child_pcb = (struct task_struct*)elem2entry(struct task_struct,all_node,p);
        if(child_pcb->parent_pid==pcb->pid){ // 存在子进程则将子进程父进程id设置为1（init进程）
            child_pcb->parent_pid = 1;
        }
        if(child_pcb->pid == pcb ->parent_pid){ //获取该进程的父进程
            parent_pcb = child_pcb;
        }
        p = p->next;
    }
    pcb->exit_status = status;
    //回收用户态资源
    releaseUserResource(pcb);
    if(parent_pcb->status == TASK_WAITING){
        thread_unblock(parent_pcb); //唤醒等待中的父进程
    }
    thread_block(TASK_HANGING); //将pcb中status设置为hanging，让父进程可以获取子进程的状态
    return 0;
}


pid_t sys_wait(int32_t * status)
{
    struct task_struct * cur_pcb = getpcb();
    struct list_elem* p = thread_all_list.head.next;
    while(1){
        struct task_struct* child_pcb=NULL;
        pid_t child_pid = 0;
        bool child_tag = false;
        while(p!=&thread_all_list.tail){
            child_pcb = (struct task_struct*)elem2entry(struct task_struct,all_node,p);
            if(child_pcb->parent_pid == cur_pcb->pid){
                if(child_pcb->status == TASK_HANGING){
                    freeKernelPage((void*)child_pcb->pgdir_vaddr); //回收子进程页目录表
                    mfree(PF_KERNEL,child_pcb->vaddr_pool.bitmap.pbitmap,DIV_ROUND_UP(child_pcb->vaddr_pool.bitmap.bitmap_byte_len,PG_SIZE));//回收子进程位图
                    *status = child_pcb->exit_status;
                    child_pid = child_pcb -> pid;
                    removePid(child_pid);
                    list_remove(&child_pcb->all_node); //在thread_all_list 中删除子进程
                    freeKernelPage(child_pcb); //回收子进程PCB //子进程资源被完全释放
                    return child_pid;
                }
                child_tag = true;
            }
            p=p->next;
        }
        if(child_tag == true){//有子进程但子进程仍在运行
            thread_block(TASK_WAITING);
        }else{ //无子进程
            return -1;
        }
        p = thread_all_list.head.next;
    }
    return -1;
}
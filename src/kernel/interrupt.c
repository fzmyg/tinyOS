#include"interrupt.h"
#include"global.h"
#include"print.h"
#include"io.h"
#include"thread.h"
#include"timer.h"
#include"keyboard.h"
#include"debug.h"
#include"console.h"
#include"string.h"
#define IDT_DESC_CNT 0x81

#define PIC_M_CTRL 0x20		//master chip's control port
#define PIC_M_DATA 0x21		//master chip's data port
#define PIC_S_CTRL 0xa0		//slave chip's control port
#define PIC_S_DATA 0xa1		//slave chip's data port

#define EFLAGES_IF 0x00000200
#define GET_EFLAGES(VAR) asm volatile("pushf;popl %0":"=g"(VAR))

static GateDesc idt[IDT_DESC_CNT];

extern int_handler int_entry_table[IDT_DESC_CNT]; /*kernel.s define every data which is address to interrupt vector*/

int_handler int_vector_table[IDT_DESC_CNT];

char * int_name[IDT_DESC_CNT];

extern void syscall_entry(void);

/* init an  idt entry */
static void mapIdtDesc(GateDesc*idte,uint8_t attribute,int_handler function)
{
	idte->func_offset_low = ((uint32_t)function&0x0000ffff);
	idte->selector = SELECTOR_K_CODE;
	idte->dcount=0;
	idte->attribute = attribute;
	idte->func_offset_high=((((uint32_t)function&0xffff0000))>>16);
}
/*init idt*/
static void idtDescInit(void)
{
	int i;
	for( i = 0;i<0x30;i++){
		mapIdtDesc(idt+i,IDT_DESC_ATTR_DPL0,int_entry_table[i]);
	}
	memset(idt+0x30,0,sizeof(GateDesc)*0x50);
	mapIdtDesc(idt+0x80,IDT_DESC_ATTR_DPL3,&syscall_entry); //初始化系统调用中断的中断门描述符
}
/*init 8295a*/
static void picInit(void)
{
	/* init master chip */
	outb(PIC_M_CTRL,ICW1_MASTER); //ICW1
	outb(PIC_M_DATA,ICW2_MASTER); //ICW2 start interrupt vector number , the low 3 bit is 0b000
	outb(PIC_M_DATA,ICW3_MASTER); //ICW3 IRQ2 connect to slave
	outb(PIC_M_DATA,ICW4_MASTER); //ICW4 not buf not auto end of interrupt
	/* init slave chip */
	outb(PIC_S_CTRL,ICW1_SLAVE); //ICW1 
	outb(PIC_S_DATA,ICW2_SLAVE); //ICW2
	outb(PIC_S_DATA,ICW3_SLAVE); //ICW3
	outb(PIC_S_DATA,ICW4_SLAVE); //ICW4
	
	outb(PIC_M_DATA,0xfc); // open master chip's IRQ0(clock) and IRQ1(keyboard) OCW1
	outb(PIC_S_DATA,0xff); // close all int from slave chip  OCW1
	put_str("pic init done\n");
}

static void generalIntHandler(uint8_t int_num)
{
	if(int_num==0x27 || int_num==0x2f)
		return;
	set_cursor(0);	
	int cursor_pos = 0;
	while(cursor_pos < 320){
		put_char(' ');
		cursor_pos ++ ;
	}
	set_cursor(0);
	put_str("======  exception message begin  =====\n");
	set_cursor(87);	
	put_str(int_name[int_num]);
	if(int_num == 0x0e){
		int page_fault_vaddr=0;
		asm volatile("movl %%cr2,%0":"=r"(page_fault_vaddr));
		put_str("\npage falult addr is ");put_int(page_fault_vaddr);
	}
	put_str("\n======  exception message end    =====\n");
	closeInt();
	while(1);
}


static void int_0x20_handler(void);

static void int_0x21_handler(void);

static void exceptionInit(void)
{
	int i = 0;
	for(i = 0;i<IDT_DESC_CNT;i++){
		int_vector_table[i]=&generalIntHandler;
	}
	registerIntFunc(0x20,&int_0x20_handler);
	registerIntFunc(0x21,&int_0x21_handler);
	int_name[0]="#DE divide error";
	int_name[1]="#DB debug exception";
	int_name[2]="#NMI not masked interrupt";
	int_name[3]="#BP breakpoint exception";
	int_name[4]="#OF overflow exception";
	int_name[5]="#BR bound range exceeded exception";
	int_name[6]="#UD invalid opcode exception";
	int_name[7]="#NM Device not available exception";
	int_name[8]="#DF double fault exception";
	int_name[9]="Coprocessor Segment Overrun";
	int_name[10]="#TS invalid TSS exception";
	int_name[11]="#NP segment not present";
	int_name[12]="#SS Stack fault exception";
	int_name[13]="#GP general protection exception";
	int_name[14]="#PF Page-Fault exception";
	//int_name[15]="#DB debug exception"; unuseed
	int_name[16]="#MF x87 FPU floationg-point error";
	int_name[17]="#AC alignment check exception";
	int_name[18]="#MC machine check exception";
	int_name[19]="#XF SIMD floating-point exception";
	int_name[0x20]="clock interrupt\n";
	int_name[0x21]="keyboadr interrupt\n";
	put_str("execptionInit done\n");
}

/*init idt , 8295a and init idtr*/
void idt_init(void)
{
	put_str("idt init start\n");
	idtDescInit();		//init idt : map interrupt vector address to every interrupt desciptor 
	picInit();		//init 8295A
	exceptionInit();	//init 
	uint64_t idt_operand = ((uint64_t)(uint32_t)idt)<<16 | (sizeof(idt)-1);
	asm volatile ("lidt %0;":"+m"(idt_operand));
	put_str("idt init done\n");
}

enum int_status getIntStatus(void)
{
	uint32_t eflags;
	GET_EFLAGES(eflags);
	if(eflags & EFLAGES_IF)
		return INT_ON;
	else
		return INT_OFF;
}

enum int_status enableInt(void)
{
	enum int_status old_status = getIntStatus();
	if(old_status == INT_OFF)
		asm volatile ("sti;");
	return old_status;
}

enum int_status closeInt(void)
{
	enum int_status old_status = getIntStatus();
	if(old_status == INT_ON)
		asm volatile ("cli":::"memory");
	return old_status;
}

enum int_status setIntStatus(enum int_status int_statu)
{
	return int_statu==INT_ON ? enableInt():closeInt();
}


static void int_0x20_handler(void)
{
	struct task_struct*cur_thread = getpcb();
        ASSERT(cur_thread->stack_magic == 0x20040104);
        cur_thread->elapsed_ticks ++;
	enum int_status stat = closeInt();
        ticks++;
	setIntStatus(stat);
        if(cur_thread->ticks==0){
                schedule();
        }else{
                cur_thread->ticks --;
        }
}

static void int_0x21_handler(void)
{	
	//enum int_status stat = closeInt();
	int_kbd_handler();
	//setIntStatus(stat);
}

void registerIntFunc(uint8_t vector_number,int_handler func)
{
	int_vector_table[vector_number] = func;
}





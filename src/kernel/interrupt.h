#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__
#include"stdint.h"

#define ICW1_IC4 1  //需设置ICW4
#define ICW1_SNGL 0 //级联
#define ICW1_ADI 0  //x86
#define ICW1_LTIM 0 //触发方式:边沿触发
#define ICW1_MASTER ((0b0001<<4) + (ICW1_LTIM<<3) + (ICW1_ADI<<2) + (ICW1_SNGL<<1) + ICW1_IC4)
#define ICW1_SLAVE  ((0b0001<<4) + (ICW1_LTIM<<3) + (ICW1_ADI<<2) + (ICW1_SNGL<<1) + ICW1_IC4)

#define ICW2_MASTER 0x20 //主片起始中断号
#define ICW2_SLAVE  0x28 //从片起始中断号

#define ICW3_MASTER 0b00000100 //IRQ2连接从片
#define ICW3_SLAVE  0x02       //主片IRQ2来连接从片

#define ICW4_SFNM 0
#define ICW4_BUF  0
#define ICW4_MS   0
#define ICW4_AEOI 0
#define ICW4_PM_x86 1
#define ICW4_MASTER ((0b000<<5)+(ICW4_SFNM<<4)+(ICW4_BUF<<3)+(ICW4_MS<<2)+(ICW4_AEOI<<1)+(ICW4_PM_x86))
#define ICW4_SLAVE  ((0b000<<5)+(ICW4_SFNM<<4)+(ICW4_BUF<<3)+(ICW4_MS<<2)+(ICW4_AEOI<<1)+(ICW4_PM_x86))
typedef void* int_handler;
/*中断门描述符*/
typedef struct GateDesc{
	uint16_t func_offset_low; //中断函数地址低16位
	uint16_t selector; 		  //中断代码段选择子
	uint8_t dcount;           //描述符固定值
	uint8_t attribute;        //描述符属性
	uint16_t func_offset_high;//中断函数地址高16位
}GateDesc;
enum int_status{
	INT_OFF,
	INT_ON
};
extern void idt_init(void);
extern enum int_status getIntStatus(void);
extern enum int_status enableInt(void);
extern enum int_status closeInt(void);
extern enum int_status setIntStatus(enum int_status);
extern void registerIntFunc(uint8_t intv_no,int_handler func);
#endif

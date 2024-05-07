#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__
#include"stdint.h"

#define ICW1_IC4 1  //need to set ICW4
#define ICW1_SNGL 0 //single or multi
#define ICW1_ADI 0  //nope
#define ICW1_LTIM 0 //edeg
#define ICW1_MASTER ((0b0001<<4) + (ICW1_LTIM<<3) + (ICW1_ADI<<2) + (ICW1_SNGL<<1) + ICW1_IC4)
#define ICW1_SLAVE  ((0b0001<<4) + (ICW1_LTIM<<3) + (ICW1_ADI<<2) + (ICW1_SNGL<<1) + ICW1_IC4)

#define ICW2_MASTER 0x20
#define ICW2_SLAVE  0x28

#define ICW3_MASTER 0b00000100
#define ICW3_SLAVE  0x02

#define ICW4_SFNM 0
#define ICW4_BUF  0
#define ICW4_MS   0
#define ICW4_AEOI 0
#define ICW4_PM_x86 1
#define ICW4_MASTER ((0b000<<5)+(ICW4_SFNM<<4)+(ICW4_BUF<<3)+(ICW4_MS<<2)+(ICW4_AEOI<<1)+(ICW4_PM_x86))
#define ICW4_SLAVE  ((0b000<<5)+(ICW4_SFNM<<4)+(ICW4_BUF<<3)+(ICW4_MS<<2)+(ICW4_AEOI<<1)+(ICW4_PM_x86))
typedef void* int_handler;

typedef struct GateDesc{
	uint16_t func_offset_low;
	uint16_t selector;
	uint8_t dcount; //const value
	uint8_t attribute;
	uint16_t func_offset_high;
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

#ifndef __KERNEL_GLOBAL_H__
#define __KERNEL_GLOBAL_H__
#include"stdint.h"


/* selector*/
#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3

#define TI_GDT 0
#define TI_LDT 1

#define SELECTOR_K_CODE ((1<<3)+(TI_GDT<<2)+RPL0)
#define SELECTOR_K_DATA ((2<<3)+(TI_GDT<<2)+RPL0)
#define SELECTOR_K_STACK SELECTOR_K_DATA
#define SELECTOR_K_GS   ((3<<3)+(TI_GDT<<2)+RPL0)

/*IDT attribute*/
#define IDT_DESC_P 1
#define IDT_DESC_DPL0 0
#define IDT_DESC_DPL3 3
#define IDT_DESC_S 0
#define IDT_DESC_32TYPE 0xe
#define IDT_DESC_16TYPE 0x6

#define IDT_DESC_ATTR_DPL0 ((IDT_DESC_P<<7)+(IDT_DESC_DPL0<<5)+(IDT_DESC_S<<4)+(IDT_DESC_32TYPE))
#define IDT_DESC_ATTR_DPL3 ((IDT_DESC_P<<7)+(IDT_DESC_DPL3<<5)+(IDT_DESC_S<<4)+(IDT_DESC_32TYPE))

/*GDT attribute*/

#define DESC_G_4K  1
#define DESC_D_32  1
#define DESC_L 0
#define DESC_AVL 0
#define DESC_P 1
#define DESC_DPL_0 0
#define DESC_DPL_1 1
#define DESC_DPL_2 2
#define DESC_DPL_3 3

#define DESC_S_CODE 1
#define DESC_S_DATA 1
#define DESC_S_SYS  0

#define DESC_CODE_TYPE 8
#define DESC_DATA_TYPE 2
#define DESC_TSS_TYPE 9

#define SELECTOR_K_CODE ((1<<3)+(TI_GDT<<2)+RPL0)
#define SELECTOR_K_DATA ((2<<3)+(TI_GDT<<2)+RPL0)
#define SELECTOR_K_STACK SELECTOR_K_DATA
#define SELECTOR_K_GS   ((3<<3)+(TI_GDT<<2)+RPL0)

#define SELECTOR_TSS ((4<<3)+(TI_GDT<<2)+RPL0)
#define SELECTOR_U_CODE ((5<<3)+(TI_GDT<<2)+RPL3)
#define SELECTOR_U_DATA ((6<<3)+(TI_GDT<<2)+RPL3)

#define GDT_ATTR_HIGH ((DESC_G_4K<<7)+(DESC_D_32<<6)+(DESC_L<<5)+(DESC_AVL<<4))
#define GDT_CODE_ATTR_LOW_DPL3 ((DESC_P<<7)+(DESC_DPL_3<<5)+(DESC_S_CODE<<4)+DESC_CODE_TYPE)
#define GDT_DATA_ATTR_LOW_DPL3 ((DESC_P<<7)+(DESC_DPL_3<<5)+(DESC_S_DATA<<4)+DESC_DATA_TYPE)

#define TSS_DESC_D 0
#define TSS_DESC_ATTR_HIGH ((DESC_G_4K<<7)+(TSS_DESC_D<<6)+(DESC_L<<5)+(DESC_AVL<<4)+0x0)
#define TSS_DESC_ATTR_LOW  ((DESC_P<<7)+(DESC_DPL_0<<5)+(DESC_S_SYS<<4)+(DESC_TSS_TYPE))

#define PG_SIZE 0x1000

#include"stdint.h"
/*全局描述符*/
struct GdtDesc{
	uint16_t limit_low_word; //低16位段界限
	uint16_t base_low_word;  //低16位段基址
	uint8_t base_mid_byte;   //中8位段基址
	uint8_t attr_low_byte;   //低8位段属性
	uint8_t limit_high_and_attr_high;//高4位段界限和高4位属性
	uint8_t base_high_byte;  //高8位段基址
};
#define DIV_ROUND_UP(a,b) ((a+b-1)/b)
#define EFLAGS_IOPL_0 (0<<12)
#define EFLAGS_MBS (1<<1)
#define EFLAGS_IF_1 (1<<9)
#define EFLAGS_IF_0 (0<<9)
#define EFLAGS_IOPL_3 (3<<12)
#endif

#include"global.h"
#include"thread.h"
#include"print.h"
#include"tss.h"
#include"string.h"
struct tss{
	uint32_t before_tss;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t blank_space;
	uint16_t io_offset;
};

static struct tss TSS;

void update_tss_esp(struct task_struct*pthread)
{
	TSS.esp0 = (uint32_t)pthread+PG_SIZE;
}

static struct GdtDesc  makeGdtDesc(uint32_t desc_addr,uint32_t limit,uint8_t attr_high,uint8_t attr_low)
{
	struct GdtDesc desc;
	desc.limit_low_word  = (uint16_t)(limit&0x0000ffff);
	desc.limit_high_and_attr_high = attr_high + ((limit&0x000f0000) >> 16);
	desc.base_low_word = (uint16_t)(desc_addr&0x0000ffff);
	desc.base_mid_byte = (uint8_t)((desc_addr&0x00ff0000)>>16);
	desc.base_high_byte = (uint8_t)((desc_addr&0xff000000)>>24);
	desc.attr_low_byte = attr_low;
	return desc;
}

void initTss()
{
	put_str("init tss start\n");
	uint32_t tss_size = sizeof(struct tss);
	memset(&TSS,0,tss_size);
	TSS.ss0 = SELECTOR_K_STACK;
	TSS.io_offset = tss_size;
	uint32_t desc = 0xc0000923;
	*(struct GdtDesc*)desc = makeGdtDesc((uint32_t)&TSS,tss_size-1,TSS_DESC_ATTR_HIGH,TSS_DESC_ATTR_LOW);
	desc+=8;
	*(struct GdtDesc*)desc = makeGdtDesc(0,0xfffff,GDT_ATTR_HIGH,GDT_CODE_ATTR_LOW_DPL3);
	desc+=8;
	*(struct GdtDesc*)desc = makeGdtDesc(0,0xfffff,GDT_ATTR_HIGH,GDT_DATA_ATTR_LOW_DPL3);
	uint64_t gdt_operand = ((8*7-1)|((uint64_t)0xc0000903)<<16);
	asm volatile ("lgdt %0"::"m"(gdt_operand));
	asm volatile ("ltr %w0"::"r"(SELECTOR_TSS));
	put_str("init tss done\n");
}



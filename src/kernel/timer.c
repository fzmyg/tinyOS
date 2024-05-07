#include"timer.h"
#include"stdint.h"
#include"print.h"
#include"io.h"
#include"thread.h"
#include"interrupt.h"

int ticks;

static void setPitFrequency(uint8_t counter_port,uint8_t counter_no,uint8_t rwl,uint8_t counter_mode,uint16_t counter_val)
{
	outb(PIT_CONTROL_PORT,(uint8_t)(counter_no << 6 | rwl <<4 | counter_mode << 1));
	outb(counter_port,(uint8_t)counter_val);
	outb(counter_port,(uint8_t)(counter_val >> 8));
}


void initTimer(void)
{
	put_str("init timer start");
	ticks = 0;
	setPitFrequency(COUNTER0_PORT,COUNTER0_NO,COUNTER0_RW,COUNTER0_MODE,COUNTER0_BASE_VALUE);
	put_str("init timer done\n");
}

#include"timer.h"
#include"stdint.h"
#include"print.h"
#include"io.h"
#include"thread.h"
#include"interrupt.h"
#include"global.h"
#define mil_seconds_per_intr (1000/IRQ0_FREQUENCY) //每次时钟中断间隔 单位:ms
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

/*按滴答数休眠当前线程(有误差)*/
static void ticks_to_sleep(uint32_t sleep_ticks)
{
	uint32_t start_tick = ticks;
	while(ticks-start_tick<sleep_ticks){
		thread_yield();
	}
}

/*以毫秒为单位休眠当前线程*/
void mtime_sleep(uint32_t m_seconds)
{
	uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds,mil_seconds_per_intr);
	ticks_to_sleep(sleep_ticks);
}
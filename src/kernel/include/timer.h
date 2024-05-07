#ifndef __TIMER_H__
#define __TIMER_H__
#define COUNTER0_PORT 0x40
#define PIT_CONTROL_PORT 0x43
#define IRQ0_FREQUENCY 100
#define CLK_FREQUENCY 1193180
#define COUNTER0_BASE_VALUE (CLK_FREQUENCY/IRQ0_FREQUENCY)
#define COUNTER0_NO 0b00
#define COUNTER0_RW 0b11
#define COUNTER0_MODE 2
extern void initTimer(void);

extern int ticks;
#endif

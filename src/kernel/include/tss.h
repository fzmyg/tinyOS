#ifndef __TSS_H__
#define __TSS_H__

#include"thread.h"

extern void update_tss_esp(struct task_struct*pthread);

extern void initTss(void);


#endif

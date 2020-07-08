#ifndef TASK_H
#define TASK_H

#include "types.h"

#define MB_4 0x400000
#define PID_AVAIL 0x1
#define MAX_PROCESS 6

uint8_t task_init();
void task_halt(uint8_t process_id);
uint32_t get_process_cnt();

// flush TLB
extern void flushTLB();

#endif /*TASK_H*/

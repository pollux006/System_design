#ifndef SIGNAL_H
#define SIGNAL_H

// #define TEST_EXTRA

#include "types.h"
#include "system_call.h"

// User-level Signals & their default actions
#define DIV_ZERO    0 // kill the task
#define SEGFAULT    1 // kill the task
#define INTERRUPT   2 // kill the task
#define ALARM       3 // Ignore
#define USER1       4 // Ignore

// handlers
#define SIG_DEFAULT signal_terminate
#define SIG_IGNORE 0

#define UNMASK_ALL 0x1F
#define CHECK_BLOCK 1

#define LOCK 1
#define UNLOCK 0

#define POP_EAX 0x58
#define MOV_SIGRT 0xB80A000000
#define INT_80 0xCD80

void sig_init(void);
void signal_generate(int32_t signum);
void signal_deliver(void);
void signal_terminate(int32_t signum);

#endif /* SIGNAL_H */

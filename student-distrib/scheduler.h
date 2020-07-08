#include "types.h"

// PIT init consts
#define PIT_PORT    0x43
#define PIT_DATA    0x40

#define PIT_IRQ     0
#define PIT_CMD     0x34    // Channel 0, Mode 2
#define HIFREQ      0x2E
#define LOFREQ      0x9B

// PIT functions
void pit_init();
void pit_handler();

// Switch process to the scheduled terminal
void switch_process(unsigned int next_ter);

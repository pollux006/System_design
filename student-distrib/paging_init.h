/* paging_init.h - Defines used to initiate paging function
 */

#include "types.h"

#ifndef _PAGING_INIT_H
#define _PAGING_INIT_H

// Define the Magic numbers for initializing paging
#define KERNEL_OFF          0x400       // The address of kernel memory base
#define VIDEO_START_OFF     0xB8        // 0xB8000 / 0x1000 = 0xB8
#define VIDEO_SIZE          0x1000      // 0x1000
#define SHIFT_OFF           12          // The offset of shifting address offest
#define MB_4_PG_OFF         22
#define MB_128_V_ADDR       0x8000000
#define MB_128_V_OFF        (MB_128_V_ADDR >> MB_4_PG_OFF)        
#define VIDEO_V_OFF         0
#define KERNEL_V_OFF        1   
#define USER_VIDEO_V_OFF    2   
#define USER_VIDEO_V_ADDR   0x8B8000    // Arbitrary picked location for video memory
#define TER_NUMBER          3
// ASM code that set page directory base pointer to PDBR(CR3) 
extern void loadPageDirectory(uint32_t*);

// ASM code that set Bit4 of CR4 to enable PSE
extern void enablePSE();

// ASM code that set Bit7 of CR4 to enable PGE
extern void enablePGE();

// ASM code that set Bit31 of CR0 to enbale PG
extern void enablePaging();

// Initialize the paging
void paging_init(void);

// Pre alloc the virtual memory address for user program
void init_user_program_pg(void);

// Pre alloc the virtual memory address for user video memory
void init_user_video_pg(void);

#endif

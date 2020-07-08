#include "types.h"
#include "x86_desc.h"

#define PCB_MASK 0xFFFFE000 // bit mask for top 8kb 
#define MB_8 0x0800000  // 8 MB
#define KB_8    0x2000 // 8 kb
#define MAX_FILE 8

#define FILE_IN_USE 1
#define FILE_NOT_IN_USE 0

#define ARG_BUF_SIZE 128

pcb_t* init_pcb(uint8_t process_id, pcb_t* parent_pcb);
pcb_t* get_pcb();
pcb_t* get_pcb_by_id(uint8_t process_id);
void store_current(pcb_t* pcb);
void restore_parent(uint8_t process_id);
fd_t* get_fa();
void close_fds();

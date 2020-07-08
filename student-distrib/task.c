#include "paging_init.h"
#include "x86_desc.h"
#include "task.h"
#include "pcb.h"
#include "lib.h"

uint32_t process_cnt = 0;  // there is always one shell
uint8_t avail_pid = 0x0;    // bit mask for available pid

/* task_init
 *
 * Allocate page for the tasks
 * Inputs: None
 * Outputs: current_id (0-indexing) for this task's pcb_t
 * Side Effects: initialize paging tables
 */
uint8_t task_init(){
  cli();
  uint8_t pid;
  if(process_cnt >= MAX_PROCESS) return -1; //support up to 6 tasks
  int i;
  // assign 
  for (i = 0; i < MAX_PROCESS; i++) { //support up to 6 tasks
    if (((avail_pid >> i) & PID_AVAIL) == 0) {
      pid = i;
      avail_pid |= (PID_AVAIL << pid);
      break;
    }
  }
  page_directory[MB_128_V_OFF].addr = (MB_8 + pid * MB_4) >> SHIFT_OFF;
  process_cnt++;
  // Flush TLB after swapping page
  flushTLB();
  return (uint8_t)pid;
}

/* task_halt
 *
 * Called when the program is halted
 * Inputs: process_id -- current pid
 * Outputs: None
 * Side Effects: initialize paging tables
 */
void task_halt(uint8_t process_id){
  pcb_t* current_pcb = get_pcb_by_id(process_id);
  // printf("destroy pid: %d\n", process_id);
  // printf("pid_avail: %x\n", avail_pid);

  avail_pid ^= (PID_AVAIL << process_id);
  // printf("mask: %x\n", PID_AVAIL << process_id);

  // printf("pid_avail: %x\n", avail_pid);

  uint8_t pid = current_pcb -> parent_id;
  page_directory[MB_128_V_OFF].addr = (MB_8 + pid * MB_4) >> SHIFT_OFF;
  // Decrement process_count
  process_cnt--;

  // Flush TLB after swapping page
  flushTLB();
  return;
}

/* get_process_cnt
 *
 * get process count
 * Inputs: None
 * Outputs: process_cnt
 * Side Effects: None
 */
uint32_t get_process_cnt (){
  return process_cnt; 
}

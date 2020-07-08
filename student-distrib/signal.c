#include "signal.h"

#ifdef TEST_EXTRA
#include "system_call.h"
#include "x86_desc.h"
#include "lib.h"
#include "pcb.h"

/* void signal_generate(int32_t signum)
 * enqueue the signal as pending
 * Inputs: signum -- number of the signal generated
 * Return Value: None
 * Function: enqueue the signal
 */
void signal_generate(int32_t signum){
  // Get the current pcb
  pcb_t* cur_pcb = get_pcb();

  // Check if the passed signal is marked as ignored
  int32_t mask = CHECK_BLOCK << signum;
  //if (!(mask & cur_pcb->mask)) return;
  // Check if the passed signal is already pending
  if (!(mask & cur_pcb->pending.mask)) return;

  cli();
  // Mask this signal as pending
  cur_pcb->pending.mask &= (!mask);
  // Enqueue the current signum and update the tail
  cur_pcb->pending.pending_signums[(int)cur_pcb->pending.tail] = signum;
  cur_pcb->pending.tail = (cur_pcb->pending.tail + 1) % NUM_SIGNALS;
  sti();
  signal_deliver();
  return;
}

/* void signal_deliver()
 * call the handler for the signal at the head of the pending queue
 * Inputs: None
 * Return Value: Nothing
 * Function: execute the signal at the head of the queue
 */
void signal_deliver(){
  pcb_t* cur_pcb = get_pcb();
  while (cur_pcb->pending.lock);
  cli();
  cur_pcb->pending.lock = 1;
  // Check if there's any pending signals
  if (cur_pcb->pending.head == cur_pcb->pending.tail) return;
  // Get the top signal from the queue
  int32_t signum = cur_pcb->pending.pending_signums[(int)cur_pcb->pending.head];
  // Update head
  cur_pcb->pending.head = (cur_pcb->pending.head + 1) % NUM_SIGNALS;
  // Unmask the signal as not pending
  cur_pcb->pending.mask |= CHECK_BLOCK << signum;
  // Double check if the signal is ignored
  if (cur_pcb->handler.handler_table[signum]){
    // TODO: Set up stackframe
    // Execute handler
    cur_pcb->handler.handler_table[signum](signum);
  }
  cur_pcb->pending.lock = 0;
  sti();
  // TODO: Call sigreturn
  return;
}


/* void signal_terminate(int32_t signum)
 * terminate the current process
 * Inputs: signum -- placeholder for handlers that require signum as an input
 * Return Value: Nothing
 * Function: terminate the current process
 */
void signal_terminate(int32_t signum){
  (void)signum;
  exception_halt();
}

#endif

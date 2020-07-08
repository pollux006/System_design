#include "pcb.h"
#include "file_sys_driver.h"
#include "lib.h"

#include "signal.h"

/* init_pcb
 *
 * Initiate pcb
 * Inputs: process_id -- the pid of new program
 *         parent_pcb -- the pointer to current pcb
 * Outputs: newpcb -- the pointer to the new pcb
 * Side Effects: initialize pcb
 */
pcb_t* init_pcb(uint8_t process_id, pcb_t* parent_pcb){
    unsigned int i;

    pcb_t* newpcb = get_pcb_by_id(process_id);
    newpcb -> current_id = process_id;
    // set entries for fda
    for (i=0;i<MAX_FILE;i++){
        newpcb -> fd_array[i].inode_idx = 0;
        newpcb -> fd_array[i].file_pos =0;
        newpcb -> fd_array[i].flag=0;
    }
    newpcb -> fd_array[0].file_op_table_ptr = &stdin_op_table;
    newpcb -> fd_array[1].file_op_table_ptr = &stdout_op_table;
    newpcb -> fd_array[0].file_pos = 0;
    newpcb -> fd_array[1].file_pos = 0;
    newpcb -> fd_array[0].inode_idx = 0;
    newpcb -> fd_array[1].inode_idx = 0;
    newpcb -> fd_array[1].flag = 1; // stdin is always in use, mark as 1
    newpcb -> fd_array[0].flag = 1; // stdout is always in use, mark as 1
    // init argument buffer
    memset(newpcb -> argument, (int32_t)'\0', ARG_BUF_SIZE);
    // if it is the first process
    if(parent_pcb == 0){
        newpcb->parent_id =0;
        newpcb->parent_pointer =0;
        newpcb->stack_bp =0;
        newpcb->stack_p =0;
        newpcb->stack_switch_bp =0;
        newpcb->stack_switch_p =0;
    }else{
        newpcb->parent_id = parent_pcb->current_id;
        newpcb->parent_pointer = (int32_t)parent_pcb;
        newpcb->stack_bp =0;
        newpcb->stack_p =0;
        newpcb->stack_switch_bp =0;
        newpcb->stack_switch_p =0;
        // store_current(newpcb);
    }
    #ifdef TEST_EXTRA
    // Initialize signal related parts
    // set the default handlers for the signals
    newpcb->handler.handler_table[DIV_ZERO] = SIG_DEFAULT;
    newpcb->handler.handler_table[SEGFAULT] = SIG_DEFAULT;
    newpcb->handler.handler_table[INTERRUPT] = SIG_DEFAULT;
    newpcb->handler.handler_table[ALARM] = SIG_IGNORE;
    newpcb->handler.handler_table[USER1] = SIG_IGNORE;

    for (i = 0; i < NUM_SIGNALS; i++){
      newpcb->pending.pending_signums[i] = 0;
    }
    newpcb->pending.head = 0;
    newpcb->pending.tail = 0;
    newpcb->pending.mask = UNMASK_ALL;
    newpcb->pending.lock = UNLOCK;
    #endif

    return newpcb;
}

/* get_pcb
 *
 * get pcb from current esp
 * Inputs: None
 * Outputs: cur_pcb -- the pointer to current pcb
 * Side Effects: None
 */
pcb_t* get_pcb(){
    uint32_t esp;
    asm volatile("movl %%esp,%0"
        : "=r"(esp)
        :
        : "memory"
    );
    return (pcb_t*)(esp & PCB_MASK);
}

/* get_pcb_by_id
 *
 * get pcb from current pid
 * Inputs: process_id -- the pid of new program
 * Outputs: cur_pcb -- the pointer to current pcb
 * Side Effects: None
 */
pcb_t* get_pcb_by_id(uint8_t process_id){
    return (pcb_t*)(MB_8-(KB_8*(process_id+1))); // pcb is stored as stack top
}

/* get_fa
 *
 * get fa from current pcb
 * Inputs: None
 * Outputs: fa_array -- pointer to current fa
 * Side Effects: None
 */
fd_t* get_fa(){
    return get_pcb() -> fd_array;
}

/* close_fds
 *
 * Close all the existing fds in fa
 * Inputs: None
 * Outputs: None
 * Side Effects: Set all fd to not in use
 */
void close_fds(){
    fd_t* fda = get_fa();
    int i;
    for(i = 0; i < MAX_FILE; i++) {
        fda[i].flag = FILE_NOT_IN_USE;
    }
    return;
}

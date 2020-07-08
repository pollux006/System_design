#include "scheduler.h"
#include "lib.h"
#include "i8259.h"
#include "system_call.h"
#include "x86_desc.h"
#include "pcb.h"
#include "task.h"
#include "paging_init.h"
#include "keyboard.h"

// pit interrupt counter 
uint32_t pit_cnt = 0;

/* pit_init
 *
 * Initiate PIT
 * Inputs: None
 * Outputs: None
 * Side Effects: initialize pit
 */
void pit_init(){
    cli();
    // Chose Mode 2 
    outb(PIT_CMD, PIT_PORT);
    // Set reload value
    // 1193182 / 100 = 11931
    outb(LOFREQ, PIT_DATA);
    outb(HIFREQ, PIT_DATA);
    // enable irq0
    enable_irq(PIT_IRQ);
    pit_cnt = 0;
    sti();
    return;
}

/* pit_handler
 *
 * pit interrupt handler
 * Inputs: None
 * Outputs: None
 * Side Effects: increase tick count, call switch process
 */
void pit_handler(){    
    // send_eoi
    send_eoi(PIT_IRQ);
    // calculate next terminal to schedule
    uint32_t next_ter = pit_cnt % TER_NUM;
    pit_cnt ++;
    cli();
    // swich process
    switch_process(next_ter);
    sti();
}


/* switch process
 *
 * save current stack and switch to the next scheduling process
 * Inputs: next_ter -- the upcomming terminal number
 * Outputs: None
 * Side Effects: save stack, switch paging and switch process
 */
void switch_process(uint32_t next_ter){
    cli();
    if (get_process_cnt()==0) execute_shell(0);
    uint8_t next_pid = terminal_pid[next_ter];
    // get current process's pcb
    pcb_t* cur_pcb = get_pcb_by_id(active_process);
    // save esp ebp
    uint32_t esp,ebp;
    asm volatile("                      \n\
            movl %%ebp,%0               \n\
            movl %%esp,%1               \n\
        "
        : "=r"(ebp), "=r"(esp)
        : 
        : "memory"
    );
    cur_pcb->stack_switch_bp=ebp;
    cur_pcb->stack_switch_p=esp;

    // now start switching
    active_process = next_pid;
    // switch video paging
    if(next_ter == active_ter){
        video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF;
        user_video_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF;
    }else{
        video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF + next_ter + 1;
        user_video_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF + next_ter + 1;
    }
    flushTLB();
    // update cursor and screen x_y 
    switch_process_cursor(cur_pcb->terminal,next_ter);
    if(next_pid == (uint8_t)-1){
        execute_shell(next_ter);
    }else{
        pcb_t* next_pcb = get_pcb_by_id(active_process);

        // switch paging
        uint32_t offset =  (MB_8 + active_process * MB_4) >> SHIFT_OFF;
        page_directory[MB_128_V_OFF].addr = offset;        
        flushTLB();

        // fetch new esp and ebp
        esp = next_pcb -> stack_switch_p;
        ebp = next_pcb -> stack_switch_bp;

        tss.esp0 = MB_8 - (active_process * KB_8) - 4;  // 4 for a line of 32bits
        tss.ss0 = KERNEL_DS;
        asm volatile("              \n\
                movl    %0, %%ebp   \n\
                movl    %1, %%esp   \n\
                leave               \n\
                ret                 \n\
                "
            :
            : "r"(ebp), "r"(esp)
            : "memory"
        );
        return;
    }
}

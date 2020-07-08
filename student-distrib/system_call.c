#include "system_call.h"
#include "x86_desc.h"
#include "pcb.h"
#include "task.h"
#include "paging_init.h"
#include "rtc.h"

#include "signal.h"

/* int32_t sys_halt(uin8_t status);
 * Inputs: status -- return value for current process
 * Return Value: 0
 * Function: restore parent process, close files and return to parent process */
int32_t sys_halt(uint8_t status) {
    cli();
    pcb_t* cur_pcb = get_pcb();
    uint8_t cur_pid = cur_pcb -> current_id;
    // if it is the last shell process, we just restart it
    if(cur_pcb->parent_pointer == 0){
        clear_terminal();
        printf("Restarting the shell...\n");
        int32_t eip = extract_ip((uint8_t*)"shell");
        iret_handler(eip);
    }
    uint8_t par_pid = cur_pcb -> parent_id;
    pcb_t* par_pcb = (pcb_t*)cur_pcb->parent_pointer;
    // restore parent paging
    task_halt(cur_pid);
    // close all the fds
    close_fds();
    active_process = par_pid;
    uint8_t par_ter = par_pcb -> terminal;
    terminal_pid[par_ter] = par_pid;
    int32_t ret_val = (int32_t) status;
    // restore parent data
    uint32_t esp = par_pcb -> stack_p;
    uint32_t ebp = par_pcb -> stack_bp;
    tss.esp0 = MB_8 - (par_pid * KB_8) - 4;
    tss.ss0 = KERNEL_DS;
    // jump to execute return
    asm volatile("              \n\
            movl    %0, %%eax   \n\
            movl    %1, %%ebp   \n\
            movl    %2, %%esp   \n\
            leave               \n\
            ret                 \n\
            "
        :
        : "r"(ret_val),"r"(ebp), "r"(esp)
        : "memory"
    );
    return 0;
}

/* int32_t sys_execute(const uint8_t* command)
 * Inputs: command -- input command
 * Return Value: Return 0;
 * Function: Initialize a new process, load the program to memory, switch to its stack.
 *           Pass real return value through EAX.
 */
int32_t sys_exeute(const uint8_t* command){
    cli();
    pcb_t* parent_pcb = get_pcb();
    if (command == NULL) return -1;
    // Parse the arguments
    // get executable file name
    uint32_t cmd_len = strlen((int8_t*)command) + 1; // handle with eol
    uint8_t fname[cmd_len];
    uint8_t arguments[cmd_len];
    memset(fname, (int32_t)'\0', cmd_len);
    memset(arguments, (int32_t)'\0', cmd_len);
    int i, j, k;
    k = 0;
    for(i = 0; i < cmd_len; i++) {
        if (command[i] == ' ') {
            if (fname[0] == '\0') continue; // ignore leading space
            else break;
        }
        if (i == (cmd_len - 1)) break;
        fname[k] = command[i];
        k++;
    }
    k = 0;
    for(j = i; j < cmd_len; j++) {
        if (command[j] == ' ' && arguments[0] == '\0') continue; // ignore leading space
        arguments[k] = command[j];
        k++;
    }
    int32_t arg_len = strlen((int8_t*)arguments);
    // Check if file is executable
    if (check_executable(fname)) return -1;
    // set up program paging
    uint8_t new_pid = task_init();
    if(new_pid==(uint8_t)-1){  // if the task init returned -1
        printf("no more than 6 process\n");
        return 2;
    }
    // load program to user stack
    load_executable(fname);
    // initiate pcb
    pcb_t* new_pcb = init_pcb(new_pid,parent_pcb);
    // printf("execute: %d, %d\n", new_pid, parent_pcb -> current_id);
    new_pcb->terminal = active_ter;
    // change termianl related
    terminal_pid[active_ter] = new_pid;
    // printf("active_ter: %d\n", active_ter);
    active_process =new_pid;
    // copy arguments
    strncpy((int8_t*)(new_pcb -> argument), (int8_t*)arguments, arg_len);
    // set esp0 and ss0
    tss.esp0 = MB_8 - (new_pid * KB_8) - 4;   //should be 4 byte above
    tss.ss0 = KERNEL_DS;
    // return from kernel mode to user mode
    int32_t eip = extract_ip(fname);
    // get current esp and ebp, save to pcb
    uint32_t esp,ebp;
    asm volatile("                      \n\
            movl %%ebp,%0               \n\
            movl %%esp,%1               \n\
        "
        : "=r"(ebp), "=r"(esp)
        :
        : "memory"
    );
    parent_pcb->stack_bp=ebp;
    parent_pcb->stack_p=esp;
    // context switch
    iret_handler(eip);
    return 0;
}

/* int32_t sys_read(int32_t fd, void* buf, int32_t nbytes)
 * Inputs: fd -- file descriptor number
 *         buf -- the buffer that takes the read data out
 *         nbytes -- the number of bytes the data suppose to read
 * Outputs: Return whatever type specific read function returns
 *          Return -1 for invalid fd
 * Side Effects: Call on the real read funtion cooresponding to file_type.
 */
int32_t sys_read(int32_t fd, void* buf, int32_t nbytes){
    if (fd == 1) return -1;
    if (fd < 0 || fd >= MAX_FILE) return -1;
    cli();
    fd_t* fda = get_fa();
    sti();
    if (!fda[fd].flag) return -1;
    return fda[fd].file_op_table_ptr -> read(fd, buf, nbytes);
}

/* int32_t sys_write(int32_t fd, void* buf, int32_t nbytes)
 * Inputs: fd -- file descriptor number
 *         buf -- the buffer that takes the data in
 *         nbytes -- the number of bytes the data suppose to write
 * Outputs: Return whatever type specific wirte function returns
 *          Return -1 for invalid fd
 * Side Effects: Call on the real write funtion cooresponding to file_type.
 */
int32_t sys_write(int32_t fd, const void* buf,int32_t nbytes){
    if (fd == 0) return -1;
    if (fd < 0 || fd >= MAX_FILE) return -1;
    cli();
    fd_t* fda = get_fa();
    if (!fda[fd].flag) return -1;
    int32_t ret = fda[fd].file_op_table_ptr -> write(fd, buf, nbytes);
    sti();
    return ret;
}

/* int32_t sys_open(const uint8_t* filename)
 * Inputs: filename -- File name
 * Outputs: Return whatever type specific open function returns
 *          Return -1 for invalid fd
 *          Return fd if file to open
 * Side Effects: Call on the real open funtion cooresponding to file_type.
 */
int32_t sys_open(const uint8_t* filename){
// allocate a free file descriptor
    fd_t* fda = get_fa();
    int32_t fdi = 2;      // file descriptor index
    while(fda[fdi].flag) {
        fdi++;
        if (fdi == FILE_NUM) return -1;
    }
    // find the dentry cooresponding to the file name
    dentry_t dentry;
    int32_t ret = read_dentry_by_name(filename, &dentry);
    if (ret == -1) return ret;
    // fill the entries of the fd
    fda[fdi].inode_idx = dentry.inode_num;
    fda[fdi].file_pos = 0;
    fda[fdi].flag = FILE_IN_USE;
    int32_t file_type = dentry.filetype;
    switch (file_type) {
    case FILE_TYPE:
        fda[fdi].file_op_table_ptr = &file_op_table;
        break;
    case DIR_TYPE:
        fda[fdi].file_op_table_ptr = &dir_op_table;
        break;
    case RTC_TYPE:
        fda[fdi].file_op_table_ptr = &rtc_op_table;
        fda[fdi].file_pos = RTC_FREQ_MAX / RTC_FREQ_MIN / TER_NUM;
        break;
    default:
        return -1;
    }
    ret = fda[fdi].file_op_table_ptr -> open(filename);
    if (!ret) return fdi;
    return ret;

}

/* int32_t sys_close(const uint8_t* filename)
 * Inputs: fd -- file descriptor number
 * Outputs: Return -1 for fail to close
 *          Return 0 for success
 * Side Effects: Call on the real close funtion cooresponding to file_type.
 *               Mark the fd as not in use.
 */
int32_t sys_close(int32_t fd){
    if (fd <= 1|| fd >= MAX_FILE) return -1;
    fd_t* fda = get_fa();
    if (!fda[fd].flag) return -1;
    int32_t ret = fda[fd].file_op_table_ptr -> close(fd);
    if (ret) return ret;
    fda[fd].flag = FILE_NOT_IN_USE;
    return 0;
}

/* sys_getargs(uint8_t* buf,int32_t nbytes)
 * Inputs: buf - the buffer that takes arguments out to user program
 *         nbytes - number of bytes to write
 * Outputs: Return -1 for invalid arguments
 *          Return 0 for success
 * Side Effects: Save arguments saved in pcb to buffer.
 */
int32_t sys_getargs(uint8_t* buf,int32_t nbytes){
    if (!buf) return -1;
    pcb_t* cur_pcb = get_pcb();
    uint8_t* arg = cur_pcb -> argument;
    int32_t arg_len = strlen((int8_t*)arg) + 1; // handle with eol
    if (arg_len == 1) return -1; // no arguments, only '\0'
    if (arg_len > nbytes) return -1; // arguments doesn't fit the buffer
    strncpy((int8_t*)buf, (int8_t*)arg, nbytes);
    return 0;
}

/* sys_vidmap(uint8_t** screen_start)
 * Inputs: screen_start
 * Outputs: Return -1 for invalid screen pointer location
 *          Return 0 for success
 * Side Effects: Save user level video memory address to pointer
 */
int32_t sys_vidmap(uint8_t** screen_start){
    if (!screen_start) return -1;
    if (KERNEL_LOW <= (int32_t)screen_start && (int32_t)screen_start < KERNEL_HIGH) return -1;
    page_directory[USER_VIDEO_V_OFF].present = 1;
    user_video_page_table[VIDEO_START_OFF].present = 1;
    *screen_start = (uint8_t*)USER_VIDEO_V_ADDR;
    return 0;
}

/* sys_set_handler()
 * Inputs: signum -- the signal number to be set
 *         handler_address -- the address of the handler
 * Outputs: Return -1 for invalid handler_address
 *          Return 0 for success
 * Side Effects: Assign new handler to the signal
 */
int32_t sys_set_handler(int32_t signum, void* handler_address){
    #ifdef TEST_EXTRA
    // Invalid signum
    if (signum < DIV_ZERO || signum > USER1) return -1;
    // Get the current pcb
    pcb_t* cur_pcb = get_pcb();
    // Reset the handler to default if handler_address is NULL
    cur_pcb->handler.handler_table[signum] = (handler_address ? handler_address : (signum <= INTERRUPT ? SIG_DEFAULT : SIG_IGNORE));
    return 0;
    #endif

    #ifndef TEST_EXTRA
    return -1;
    #endif
}

/* sys_sigreturn()
 * Not implemented yet */
int32_t sys_sigreturn(void){
    return -1;
}

/* int32_t execute_shell();
 * Inputs: None
 * Return Value: Return 0
 * Function: Execute shell in kernel.c as the first user program.
 */
int32_t execute_shell(uint32_t ter){
    cli();
    if (ter == 0){
        clear_terminal();
        // initialize to -1 to indicate no process running
        terminal_pid[0] = -1;
        terminal_pid[1] = -1;
        terminal_pid[2] = -1;
    }
    uint8_t* fname =(uint8_t*)"shell";
    // set up program paging
    uint8_t new_pid = task_init();
    // load program to user stack
    load_executable(fname);
    // initiate pcb
    pcb_t* new_pcb = init_pcb(new_pid,0);
    new_pcb->terminal=ter;
    active_process = new_pid;
    terminal_pid[ter]=new_pid;
    // set esp0 and ss0
    tss.esp0 =  MB_8 - (new_pid * KB_8) - 4;   //should be 4 byte above
    tss.ss0 = KERNEL_DS;
    // return from kernel mode to user mode
    int32_t eip = extract_ip(fname);
    iret_handler(eip);
    return 0;
}

/* int32_t sys_halt();
 * Inputs: None
 * Return Value: None
 * Function: Handle the halt causing by exeception,
 *           Functionning the same as sys_halt but always return 256 to execute
 */
int32_t exception_halt(void) {
    cli();
    pcb_t* cur_pcb = get_pcb();
    uint8_t cur_pid = cur_pcb -> current_id;
    uint8_t par_pid = cur_pcb -> parent_id;
    pcb_t* par_pcb = (pcb_t*)cur_pcb->parent_pointer;
    active_process = par_pid;
    uint8_t par_ter = par_pcb -> terminal;
    terminal_pid[par_ter] = par_pid;
    // restore parent paging
    task_halt(cur_pid);
    // close all the fds
    close_fds();
    int32_t ret_val = EXECPTION_RET;
    // restore parent data
    uint32_t esp = par_pcb -> stack_p;
    uint32_t ebp = par_pcb -> stack_bp;
    tss.esp0 = MB_8 - (par_pid * KB_8) - 4;
    tss.ss0 = KERNEL_DS;
    // jump to execute return
    asm volatile("              \n\
            movl    %0, %%eax   \n\
            movl    %1, %%ebp   \n\
            movl    %2, %%esp   \n\
            leave               \n\
            ret                 \n\
            "
        :
        : "r"(ret_val),"r"(ebp), "r"(esp)
        : "memory"
    );
    return 0;
}

/* int32_t sys_halt();
 * Inputs: None
 * Return Value: None
 * Function: Handle the halt causing by exeception,
 *           Functionning the same as sys_halt but always return 256 to execute
 */
int32_t keyboard_halt(uint8_t active_ter) {
    cli();
    uint8_t halt_pid = terminal_pid[active_ter];
    pcb_t* cur_pcb = get_pcb_by_id(halt_pid);
    uint8_t cur_pid = cur_pcb -> current_id;
    uint8_t par_pid = cur_pcb -> parent_id;
    pcb_t* par_pcb = (pcb_t*)cur_pcb->parent_pointer;
    active_process = par_pid;
    uint8_t par_ter = par_pcb -> terminal;
    terminal_pid[par_ter] = par_pid;
    // restore parent paging
    task_halt(cur_pid);
    // close all the fds
    close_fds();
    int32_t ret_val = EXECPTION_RET;
    // restore parent data
    uint32_t esp = par_pcb -> stack_p;
    uint32_t ebp = par_pcb -> stack_bp;
    tss.esp0 = MB_8 - (par_pid * KB_8) - 4;
    tss.ss0 = KERNEL_DS;
    // jump to execute return
    asm volatile("              \n\
            movl    %0, %%eax   \n\
            movl    %1, %%ebp   \n\
            movl    %2, %%esp   \n\
            leave               \n\
            ret                 \n\
            "
        :
        : "r"(ret_val),"r"(ebp), "r"(esp)
        : "memory"
    );
    return 0;
}

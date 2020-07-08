/* interrupt_wrapper.S - ASM functions used ro wrap the
 * handlers.
 */
#define ASM 1

#include "x86_desc.h"

/* Save all the registers on stack*/
.macro SAVE_ALL
    cld
    pushl   %fs
    pushl   %es
    pushl   %ds
    pushl   %eax
    pushl   %ebp
    pushl   %edi
    pushl   %esi
    pushl   %edx
    pushl   %ecx
    pushl   %ebx
.endm

/* Restore all the registers from stack*/
.macro RESTORE_ALL
    popl    %ebx
    popl    %ecx
    popl    %edx
    popl    %esi
    popl    %edi
    popl    %ebp
    popl    %eax
    popl    %ds
    popl    %es
    popl    %fs
    addl    $4, %esp
.endm

.data
    NR_syscalls = 10            # number of system calls
    ENOSYS = 1                  # error number
    MB_132_V_ADDR = 0x83ffffc   # User-stack ESP

.text
.global keyboard_handler_asm, rtc_handler_asm, rtc_test_handler_asm,syscall_handler_asm,iret_handler, pit_handler_asm

sys_call_table:
    .long 0
    .long sys_halt
    .long sys_exeute
    .long sys_read
    .long sys_write
    .long sys_open
    .long sys_close
    .long sys_getargs
    .long sys_vidmap
    .long sys_set_handler
    .long sys_sigreturn

/* keyboard_handler_asm
 *
 * Interrupt wrapper for keyboard_handler
 * Inputs: None
 * Outputs: None
 * Side Effects: call keyboard_handler
 */
keyboard_handler_asm:
    call    keyboard_handler
    iret

/* rtc_handler_asm
 *
 * Interrupt wrapper for rtc_handler
 * Inputs: None
 * Outputs: None
 * Side Effects: call rtc_handler
 */
rtc_handler_asm:
    call    rtc_handler
    iret


/* pit_handler_asm
 *
 * Interrupt wrapper for pit_handler
 * Inputs: None
 * Outputs: None
 * Side Effects: call pit_handler
 */
pit_handler_asm:
    call    pit_handler
    iret

/* syscall_handler_asm
 *
 * Systemcall wrapper for system_call_handler
 * Inputs: register based input
 *          ebx,ecx,edx -> three parameters
 *          eax -> number of which system_call to call
 * Outputs: eax -> the return num of syscall or -1 for invalid call
 * Side Effects: call according to the number of system call
 */
syscall_handler_asm:
    pushl   %eax
    SAVE_ALL

    # check eax number
    cmpl    $NR_syscalls, %eax              # system call num are from 1 to 10
    ja      badsys

    cmpl    $0, %eax                        # system call cannot be 0
    je      badsys

    # do the system call
    sti
    call    *sys_call_table(, %eax, 4)
    cli
    movl    %eax, 24(%esp)                 # store return code on the stack at eax's position
    jmp     syscall_ret

badsys:
    # deal with bad syscall
    movl    $(-ENOSYS), 24(%esp)           # move error number to eax

syscall_ret:
    RESTORE_ALL
    iret

/* iret_handler
 *
 * Inputs:  eip -- the entry point for user program
 * Outputs: None
 * Side Effects: Switch stack
 */
iret_handler:
    cli
    movl    4(%esp),%eax    # function handler

    movl    $USER_DS, %ecx  #  get data segment
    movw    %cx,%ds

    pushl   %ecx            # push SS
    pushl   $MB_132_V_ADDR  # push esp
    pushfl                  # push flag

    popl    %ecx
    orl     $0x200, %ecx    # set IF bits 0x200
    pushl   %ecx

    pushl   $USER_CS        # push CS
    pushl   %eax            # push eip
    
    iret    

/* rtc_test_handler_asm
 *
 * Interrupt wrapper for rtc_test_handler
 * Inputs: None
 * Outputs: None
 * Side Effects: call rtc_test_handler
 */
rtc_test_handler_asm:
    cli
    pushl   %ebp
    movl    %esp, %ebp
    call    rtc_test_handler
    leave
    sti
    iret

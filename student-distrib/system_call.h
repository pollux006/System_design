#ifndef SYSTEM_CALL_H
#define SYSTEM_CALL_H

#include "types.h"
#include "file_sys_driver.h"

#define FILE_NUM 8
#define FILE_IN_USE 1
#define FILE_NOT_IN_USE 0

#define EXECPTION_RET 256

// Kernel Range
#define KERNEL_LOW  0x400000
#define KERNEL_HIGH 0x800000

// multi-terminal parameters
uint8_t terminal_pid[TER_NUM];
uint8_t active_process;
uint8_t active_ter;

// syscall hanlders
int32_t sys_halt(uint8_t status);
int32_t sys_exeute(const uint8_t* command);
int32_t sys_read(int32_t fd, void* buf, int32_t nbytes);
int32_t sys_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t sys_open(const uint8_t* filename);
int32_t sys_close(int32_t fd);
int32_t sys_getargs(uint8_t* buf,int32_t nbytes);
int32_t sys_vidmap(uint8_t** screen_start);
int32_t sys_set_handler(int32_t signum, void* handler_address);
int32_t sys_sigreturn(void);

// special syscalls
int32_t execute_shell(uint32_t ter);
int32_t exception_halt(void);
int32_t keyboard_halt(uint8_t active_ter);

// switch context
extern void iret_handler();

#endif /*SYSTEM_CALL_H*/

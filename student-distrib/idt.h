/* idt.h - Defines used in initializing the IDT
 */

#ifndef _IDT_H
#define _IDT_H

#ifndef ASM

// Initialize the IDT
void init_idt();
void enable_rtc_test();
// ASM wrappers for Hardware interrupt handlers
extern void keyboard_handler_asm();
extern void rtc_handler_asm();
extern void rtc_test_handler_asm();
extern void syscall_handler_asm();
extern void pit_handler_asm();

#endif

// Vector offsets for several entries
#define SYSTEM_CALL_VEC 0x80
#define TIMER_VEC       0x20
#define KEYBOARD_VEC    0x21
#define RTC_VEC         0x28

#endif /* _IDT_H */

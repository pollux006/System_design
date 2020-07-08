/* idt.c - Functions used in initializing the IDT
 */

#include "idt.h"
#include "x86_desc.h"
#include "lib.h"
#include "keyboard.h"
#include "rtc.h"
#include "tests.h"
#include "terminal_driver.h"
#include "types.h"
#include "system_call.h"

#include "signal.h"

#ifndef TEST_EXTRA
// Define Handler of Exceptions
#define EXCEPTION(function,string)          \
void function(){                            \
    printf("%s\n",#string);                 \
    exception_halt();                       \
}

// Exception handlers
EXCEPTION(EX_DE,"Divide Error Exception");
EXCEPTION(EX_DB,"Debug Exception");
EXCEPTION(EX_NMI,"NMI Interrrupt");
EXCEPTION(EX_BP,"Breakpoint Exception");
EXCEPTION(EX_OF,"Overflow Exception");
EXCEPTION(EX_BR,"BOUND Range Exceeded Exception");
EXCEPTION(EX_UD,"Invalid Opcode Exception");
EXCEPTION(EX_NM,"Device not avaliable Exception");
EXCEPTION(EX_DF,"Double Fault Exception");
EXCEPTION(EX_CSO,"Coprocessor Segment Overrun");
EXCEPTION(EX_TS,"Invalid TSS Exception");
EXCEPTION(EX_NP,"Segment Not Present");
EXCEPTION(EX_SS,"Stack Fault Exception");
EXCEPTION(EX_GP,"General Protection Exception");
EXCEPTION(EX_PF,"Page-Fault Exception");
/*do not use #15 intel reserved*/
EXCEPTION(EX_MF,"x87 FPU Floating-Point Error");
EXCEPTION(EX_AC,"Alignment Check Exception");
EXCEPTION(EX_MC,"Machine-Check Exception");
EXCEPTION(EX_XF,"SIMD Floating-Point Exception");

EXCEPTION(SYS_CALL,"System call triggered");

EXCEPTION(EX_ND,"Not Defined");   // used for an not defined interrupt

#endif

#ifdef TEST_EXTRA
// Define Handler for Exceptions that generates DIV_ZERO signal
#define EXCEPTION_DIV_ZERO(function,string)          \
void function(){                            \
    signal_generate(DIV_ZERO);              \
}

// Define Handler for Exceptions that generates SEGFAULT signal
#define EXCEPTION_SEGFAULT(function,string)          \
void function(){                            \
    signal_generate(SEGFAULT);              \
}

// Exception handlers
EXCEPTION_DIV_ZERO(EX_DE,"Divide Error Exception");
EXCEPTION_SEGFAULT(EX_DB,"Debug Exception");
EXCEPTION_SEGFAULT(EX_NMI,"NMI Interrrupt");
EXCEPTION_SEGFAULT(EX_BP,"Breakpoint Exception");
EXCEPTION_SEGFAULT(EX_OF,"Overflow Exception");
EXCEPTION_SEGFAULT(EX_BR,"BOUND Range Exceeded Exception");
EXCEPTION_SEGFAULT(EX_UD,"Invalid Opcode Exception");
EXCEPTION_SEGFAULT(EX_NM,"Device not avaliable Exception");
EXCEPTION_SEGFAULT(EX_DF,"Double Fault Exception");
EXCEPTION_SEGFAULT(EX_CSO,"Coprocessor Segment Overrun");
EXCEPTION_SEGFAULT(EX_TS,"Invalid TSS Exception");
EXCEPTION_SEGFAULT(EX_NP,"Segment Not Present");
EXCEPTION_SEGFAULT(EX_SS,"Stack Fault Exception");
EXCEPTION_SEGFAULT(EX_GP,"General Protection Exception");
EXCEPTION_SEGFAULT(EX_PF,"Page-Fault Exception");
/*do not use #15 intel reserved*/
EXCEPTION_SEGFAULT(EX_MF,"x87 FPU Floating-Point Error");
EXCEPTION_SEGFAULT(EX_AC,"Alignment Check Exception");
EXCEPTION_SEGFAULT(EX_MC,"Machine-Check Exception");
EXCEPTION_SEGFAULT(EX_XF,"SIMD Floating-Point Exception");

EXCEPTION_SEGFAULT(EX_ND,"Not Defined");   // used for an not defined interrupt
#endif

/* init_idt
 *
 * Fill in every entry on the IDT
 * Inputs: None
 * Outputs: None
 * Side Effect: initialize IDT, prepare to handle the interrupts
 */
void init_idt(){
    unsigned int i; // Loop counter

    // Traverse through all the IDT entries
    for (i = 0; i < NUM_VEC; i++){
        if (i == SYSTEM_CALL_VEC)
            idt[i].dpl = 3;         // A system call's privilege is 3
        else
             idt[i].dpl = 0;        // Hardward interrupt and Exception have privilege of 0

        // Fill all the bits beside the handler offset
        idt[i].seg_selector = KERNEL_CS;
        idt[i].present = 0x1;
        idt[i].size = 0x1;
        idt[i].reserved0 = 0x00;
        idt[i].reserved1 = 0x01;
        idt[i].reserved2 = 0x01;
        idt[i].reserved3 = 0x00;
        idt[i].reserved4 = 0x00;
        // Fill all the non-Exception entries as not defined
        SET_IDT_ENTRY(idt[i], EX_ND);       // Not defined
    }


    // Set IDT
    // Fill Exceptions
    SET_IDT_ENTRY(idt[0], EX_DE);
    SET_IDT_ENTRY(idt[1], EX_DB);
    SET_IDT_ENTRY(idt[2], EX_NMI);
    SET_IDT_ENTRY(idt[3], EX_BP);
    SET_IDT_ENTRY(idt[4], EX_OF);
    SET_IDT_ENTRY(idt[5], EX_BR);
    SET_IDT_ENTRY(idt[6], EX_UD);
    SET_IDT_ENTRY(idt[7], EX_NM);
    SET_IDT_ENTRY(idt[8], EX_DF);
    SET_IDT_ENTRY(idt[9], EX_CSO);
    SET_IDT_ENTRY(idt[10], EX_TS);
    SET_IDT_ENTRY(idt[11], EX_NP);
    SET_IDT_ENTRY(idt[12], EX_SS);
    SET_IDT_ENTRY(idt[13], EX_GP);
    SET_IDT_ENTRY(idt[14], EX_PF);
    // #15 is reserved by Intel
    SET_IDT_ENTRY(idt[16], EX_MF);
    SET_IDT_ENTRY(idt[17], EX_AC);
    SET_IDT_ENTRY(idt[18], EX_MC);
    SET_IDT_ENTRY(idt[19], EX_XF);

    // Add Hardware Interrupt
    SET_IDT_ENTRY(idt[TIMER_VEC], pit_handler_asm);
    SET_IDT_ENTRY(idt[KEYBOARD_VEC], keyboard_handler_asm);
    SET_IDT_ENTRY(idt[RTC_VEC], rtc_handler_asm);

    // System Call: will be implemented in the future
    SET_IDT_ENTRY(idt[SYSTEM_CALL_VEC], syscall_handler_asm);

    // Load IDTR
    lidt(idt_desc_ptr);
}

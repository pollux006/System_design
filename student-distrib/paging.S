/*
 * paging.h - Functions to initiate paging function
 */

.data
    PG_ENABLE = 0x80000000      # Bit mask for MSB of CR0
    PSE_ENABLE = 0x10           # Bit mask for Bit4 of CR04
    PGE_ENABLE = 0x80           # Bit mask for Bit7 of CR04

.text
.globl loadPageDirectory, enablePaging, enablePSE, enablePGE, flushTLB

/* loadPageDirectory
 *
 * Load page directory base pointer to CR3
 * Inputs: page directory base pointer
 * Outputs: None
 * Side Effects: Load page directory base pointer to CR3
 */
loadPageDirectory:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%esp), %eax
    movl    %eax, %cr3
    leave
    ret

/* enablePaging
 *
 * Set the MSB of CR0 to 1 to enable paging
 * Inputs: None
 * Outputs: None
 * Side Effects: Set the MSB of CR0 to 1 to enable paging
 */
enablePaging:
    pushl   %ebp
    movl    %esp, %ebp
    movl    %cr0, %eax
    orl     $PG_ENABLE, %eax     # Set the MSB of CR0
    movl    %eax, %cr0
    leave
    ret

/* enablePSE
 *
 * Set the Bit4 of CR4 to 1 to enable paging extension
 * Inputs: None
 * Outputs: None
 * Side Effects: Set the Bit4 of CR4 to 1 to enable paging extension
 */
enablePSE:
    pushl   %ebp
    movl    %esp, %ebp
    movl    %cr4, %eax
    orl     $PSE_ENABLE, %eax     # Set the Bit4 of CR4
    movl    %eax, %cr4
    leave
    ret

/* enablePGE
 *
 * Set the Bit7 of CR4 to 1 to enable paging extension
 * Inputs: None
 * Outputs: None
 * Side Effects: Set the Bit7 of CR4 to 1 to enable paging extension
 */
enablePGE:
    pushl   %ebp
    movl    %esp, %ebp
    movl    %cr4, %eax
    orl     $PGE_ENABLE, %eax     # Set the Bit7 of CR4
    movl    %eax, %cr4
    leave
    ret

/* flushTLB
 *
 * Write to the page directory base register (CR3)
 * Inputs: None
 * Outputs: None
 * Side Effects: Flush the TLB
 */
flushTLB:
    movl    %cr3, %eax
    movl    %eax, %cr3
    ret

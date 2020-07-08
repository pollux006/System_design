/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = MASTER_MAKS; /* IRQs 0-7  */
uint8_t slave_mask = MASK;  /* IRQs 8-15 */

/* i8259_init
 *
 * Initialize the 8259 PIC
 * Inputs: None
 * Outputs: None
 * Side Effects: initialize the 8259 PIC
 */
void i8259_init(void) {
    /* clear masks on pic */
    outb(MASK, MASTER_8259_DATA);
    outb(MASK, SLAVE_8259_DATA);

    /* initialize master pic */
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW2_MASTER, MASTER_8259_DATA);
    outb(ICW3_MASTER, MASTER_8259_DATA);
    outb(ICW4, MASTER_8259_DATA);

    /* initialize slave pic */
    outb(ICW1, SLAVE_8259_PORT);
    outb(ICW2_SLAVE, SLAVE_8259_DATA);
    outb(ICW3_SLAVE, SLAVE_8259_DATA);
    outb(ICW4, SLAVE_8259_DATA);

    /* set mask */
    outb(master_mask, MASTER_8259_DATA);
    outb(slave_mask, SLAVE_8259_DATA);
}

/* enable_irq
 *
 * Enable (unmask) the specified IRQ
 * Inputs: irq_num -- the irq number
 * Outputs: None
 * Side Effects: enable the corresponding irq
 */
void enable_irq(uint32_t irq_num) {
    /* check if the input irq is connected directly to the master or to the slave */
    if (irq_num < SLAVE_OFFSET) {
        /* unmask the corresponding bit on master */
        master_mask &= ~(BIT << irq_num);
        outb(master_mask, MASTER_8259_DATA);
    } else {
        /* unmask the corresponding bit on slave */
        slave_mask &= ~(BIT << (irq_num - SLAVE_OFFSET));
        outb(slave_mask, SLAVE_8259_DATA);
    }
}

/* disable_irq
 *
 * Disable (mask) the specified IRQ
 * Inputs: irq_num -- the irq number
 * Outputs: None
 * Side Effects: disable the corresponding irq
 */
void disable_irq(uint32_t irq_num) {
    /* check if the input irq is connected directly to the master or to the slave */
    if (irq_num < SLAVE_OFFSET) {
        /* mask the corresponding bit on master */
        master_mask |= (BIT << irq_num);
        outb(master_mask, MASTER_8259_DATA);
    } else {
        /* mask the corresponding bit on slave */
        slave_mask |= (BIT << (irq_num - SLAVE_OFFSET));
        outb(slave_mask, SLAVE_8259_DATA);
    }
}

/* send_eoi
 *
 * Send end-of-interrupt signal for the specified IRQ
 * Inputs: irq_num -- the irq number
 * Outputs: None
 * Side Effects: send the end of interrupt message for the corresponding irq
 */
void send_eoi(uint32_t irq_num) {
    /* check if the input irq is connected directly to the master or to the slave */
    if (irq_num < SLAVE_OFFSET) {
        /* send the corresponding EOI message to the master */
        outb(EOI|irq_num, MASTER_8259_PORT);
    } else {
        /* send the corresponding EOI message to the slave first */
        outb(EOI|(irq_num - SLAVE_OFFSET), SLAVE_8259_PORT);
        /* and then send the corresponding EOI message to the master */
        outb(EOI|ICW3_SLAVE, MASTER_8259_PORT);
    }
}

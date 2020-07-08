/* rtc.c - Functions used to initiate rtc
 */

#include "rtc.h"
#include "i8259.h"
#include "lib.h"
#include "x86_desc.h"
#include "pcb.h"

#include "signal.h"

#define TEST_RTC

// global flag to check if an interrupt occured
volatile int interrupt_flag = 0;

// Virualized RTC Counter
int32_t tick_counter = 0;

// global counter to trigger ALARM signal once every 10 seconds
// cleared everytime when change_rate() is called, and increments everytime rtc_handler is triggered
int timer;

/* rtc_init
 *
 * initialize rtc, enable rtc interrupt
 * Inputs: None
 * Outputs: None
 * Side Effects: enable rtc interrupt
 */
void rtc_init() {
    // Enbale periodic interrupt
    outb(0x8B, RTC_PORT);           // Select register B, disable NMI
    char b_val = inb(RTC_DATA);     // Get the value saved in register B
    char new_b = b_val | PIE_BIT;   // Set the bit for PIE
    outb(0x8B, RTC_PORT);           // Select register B, disable NMI, again
    outb(~new_b, RTC_DATA);          // Write the new value back ro register B
    outb(new_b, RTC_DATA);          // Write the new value back ro register B

    // Low the frequency of RTC for testing purpose
    // frequceny = 32768 >> (rate - 1)
    // rate = 7 for frequency = 512Hz
    change_rate(RTC_RATE_MIN);
    tick_counter = 0;
    // Enable IRQ8
    enable_irq(RTC_IRQ);
}

/* rtc_handler
 *
 * rtc interrupt handler
 * Inputs: None
 * Outputs: None
 * Side Effects: increase tick count, set flag
 */
void rtc_handler() {
    // Send EOI
    send_eoi(RTC_IRQ);
    // Using Virtualized RTC
    tick_counter++;
    interrupt_flag = 1;
    #ifdef TEST_EXTRA
    if (tick_counter % (RTC_FREQ_MAX * SIG_INTERVAL) == 0) signal_generate(ALARM);
    #endif
    // Throw away the data saved in register C
    outb(0x0C, RTC_PORT);
    inb(RTC_DATA);
}

/* rtc_open
 *
 * Inputs: filename -- not used
 * Outputs: Always 0
 * Side Effects: None
 */
int32_t rtc_open(const uint8_t* fname){
    // Using Virtualized RTC
    return 0;
}

/* rtc_close
 *
 * Inputs: fd -- file descriptor
 * Outputs: Always 0
 * Side Effects: set frequency back to 2 Hz
 */
int32_t rtc_close(int32_t fd){
    fd_t* fda = get_fa();
    fda[fd].file_pos = RTC_FREQ_MAX / RTC_FREQ_MIN / TER_NUM;
    return 0;
}

/* rtc_read
 *
 * RTC read blocks the program until an virtualized interrupt is received
 * Inputs: fd -- file descriptor
 *         buf -- Not used
 *         nbytes -- Not used
 * Outputs:  Always 0
 * Side Effects: Set flag
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
    fd_t* fda = get_fa();
    while(!interrupt_flag);
    while(tick_counter % fda[fd].file_pos != 0);
    interrupt_flag = 0;
    return 0;
}

/* rtc_write
 *
 * RTC write writes a new frequency.
 * Inputs: fd -- file descriptor
 *         buf -- hold the input frequency
 *         nbytes -- should always be 4
 * Outputs: Return 0 for success;
 *          return -1 for invalid frequency input
 * Side Effects: Write the virtulized frequency to rtc's fd
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
    fd_t* fda = get_fa();
    int32_t freq_to_write;
    if((nbytes != sizeof(int)) || buf == NULL) return -1; //invalid input
    freq_to_write = *((int32_t*)buf);
    // Check if freq is in correct range and is the power of 2
    if (freq_to_write > RTC_FREQ_MAX || freq_to_write < RTC_FREQ_MIN || (freq_to_write & (freq_to_write - 1)) != 0) return -1;
    // Using virtualized RTC
    fda[fd].file_pos = RTC_FREQ_MAX / freq_to_write / TER_NUM;
    return 0;
}

/* change_rate
 *
 * Change the rate for RTC by RS0-3 setting register A
 * Inputs: rate - int bigger than 3 and smaller or equal to 15 that
 *                specifies the rate of RTC
 * Outputs: None
 * Side Effects: change the update rate of rtc
 */
void change_rate(int rate){
    rate &= LOW_HB_MASK;                // rate cannot larger than 15
                                        // frequceny = 32768 >> (rate - 1)
    if (rate >= RTC_RATE_MIN){
        cli();
        timer = 0;                      // Clear the timer
        outb(0x8A, RTC_PORT);           // Select register A, disable NMI
        char a_val = inb(RTC_DATA);     // Get the value saved in register A
        char new_a = (a_val & HIGH_HB_MASK) | rate;   // Set the bit for RS0-3
        outb(0x8A, RTC_PORT);           // Select register A, disable NMI, again
        outb(~new_a, RTC_DATA);          // Disable the periodic count first to reset the cycle
        outb(new_a, RTC_DATA);          // Write the new value back ro register A
        sti();
    }
}

#ifdef TEST_RTC

/* rtc_test_handler
 *
 * rtc interrupt handler with test_interrupt enabled
 * Inputs: None
 * Outputs: None
 * Side Effects: update every character on the screen
 */
void rtc_test_handler() {
    // Throw away the data saved in register C
    outb(0x0C, RTC_PORT);
    inb(RTC_DATA);
    // Call test_interrupt()
    test_interrupts();
    // Send EOI
    send_eoi(RTC_IRQ);
}

#endif

/* rtc.h - Defines used to initiate RTC
 */

#include "types.h"

#ifndef _RTC_H
#define _RTC_H

// Define Magic numbers used for initializing RTC
#define RTC_IRQ         8
#define RTC_PORT        0x70
#define RTC_DATA        0x71
#define PIE_BIT         0x40
#define LOW_HB_MASK     0x0F
#define HIGH_HB_MASK    0xF0
#define FREQ_TO_RATE    0x8000
#define RTC_RATE_MAX    15
#define RTC_RATE_MIN    6
#define RTC_FREQ_MAX    1024
#define RTC_FREQ_MIN    2
#define SIG_INTERVAL    10
// Enable RTC interrrupt on PIC
void rtc_init();

// RTC interrupt handler
void rtc_handler();

// RTC interrupt handler for tests
void rtc_test_handler();

// RTC system call
int32_t rtc_open(const uint8_t* fname);
int32_t rtc_close(int32_t fd);
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

// Change the rate of RTC
void change_rate(int rate);

#endif

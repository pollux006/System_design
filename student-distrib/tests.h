#ifndef TESTS_H
#define TESTS_H

// test launcher
void launch_tests();

#define TEST_PERIOD 3

// Video memory offset
#define VIDEO_ADDR_1 0xB8000
#define VIDEO_ADDR_2 0xB8800
#define VIDEO_ADDR_3 0xB8FFC
#define VIDEO_ADDR_4 0xB7FFF
#define VIDEO_ADDR_5 0xB9001

// Kernel offset
#define KENREL_ADDR_1 0x400000
#define KENREL_ADDR_2 0x600000
#define KENREL_ADDR_3 0x7FFFFC
#define KENREL_ADDR_4 0x3FFFFF
#define KENREL_ADDR_5 0x800001

// Terminal tests
#define TERMINAL_PRINT_TEST 22
#define TERMINAL_WRITE_TEST 10

// Buffer size
#define DIR_BUF 32
#define FIL_BUF 0x10000
#define FIL_BUF_EDGE 10
#define GREP 3
#define LS 12
#define FISH 6
#define VERYLARGE 11
#define FRAME0 10
#define FRAME1 15

// Print Format
#define NAME_SIZE 32
#define LEN_SIZE 6

// rtc frequency
#define INITIAL_FREQ 2
#define RTC_TEST_NUM 9
#define RTC_TEST_LENGTH 5

#define RTC_INVALID_1 2048
#define RTC_INVALID_2 1
#define RTC_INVALID_3 433
#define RTC_VALID_1 2
#define RTC_VALID_2 1024

#endif /* TESTS_H */

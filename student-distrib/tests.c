#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "rtc.h"
#include "keyboard.h"
#include "idt.h"
#include "terminal_driver.h"
#include "file_sys_driver.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}

/* enable all tests in one run */
void test_wrapper_no_param(char* str, int (*func)()){
	int i;
	TEST_OUTPUT(str, func());
	(void)rtc_open(0);
	for (i = 0; i < TEST_PERIOD; i++){
		(void)rtc_read(0,0,0);                   // Leave 1.5s for each test to execute
	}
	printf("Press enter to execute the next test\n");
	while (end_test()); 									// Press enter to end the test
	(void)rtc_close(0);
	clear();
}

void test_wrapper_int(char* str, int (*func)(int), int param){
	int i;
	TEST_OUTPUT(str, func(param));
	(void)rtc_open(0);
	for (i = 0; i < TEST_PERIOD; i++){
		(void)rtc_read(0,0,0);                   // Leave 1.5s for each test to execute
	}
	printf("Press enter to execute the next test\n");
	while (end_test()); 									// Press enter to end the test
	(void)rtc_close(0);
	clear();
}
/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that all IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;
	int i;
	int result = PASS;
	for (i = 0; i < NUM_VEC; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}
	return result;
}

// add more tests here
// Exception tests
/* DE_test
 *
 * Trigger Divide-by-zero error using 1 / 0
 * Inputs: None
 * Outputs: Freezed screen with a string showing the exception/FAIL
 * Side Effects: None
 * Coverage: Divide-by-zero exception handler
 * Files: idt.h/.c
 */
int DE_test(){
	TEST_HEADER;
	int a = 0;
	int b = 1 / a; // Trigger here
	printf("%d", b);
	return FAIL; // The test should never reach here
}

/* OF_test
 *
 * Trigger Overflow error using add
 * Inputs: None
 * Outputs: Freezed screen with a string showing the exception/FAIL
 * Side Effects: None
 * Coverage: Overflow exception handler
 * Files: idt.h/.c
 */
int OF_test(){
	TEST_HEADER;
	asm ("							  \n\
			movl $0x7fffffff, %eax    \n\
			addl %eax, %eax			  \n\
			into					  \n\
		");
	return FAIL; 		// The test should never reach here
}

/* UD_test
 *
 * Trigger undefined opcode error using an undefined opcode
 * Inputs: None
 * Outputs: Freezed screen with a string showing the exception/FAIL
 * Side Effects: None
 * Coverage: undefined opcode exception handler
 * Files: idt.h/.c
 */
int UD_test(){
	TEST_HEADER;
	asm ("RSM"); // Use an undefined opcode
	return FAIL; // The test should never reach here
}

/* PF_test
 *
 * Trigger paging fault error by dereferencing an uninitialized memory address
 * Inputs: None
 * Outputs: Freezed screen with a string showing the exception/FAIL
 * Side Effects: None
 * Coverage: paging fault exception handler
 * Files: idt.h/.c
 */
int PF_test(){
	TEST_HEADER;
	uint32_t content = *((uint32_t*)0x0); // check nullptr
	printf("%d", content);
	return FAIL; 						// The test should never reach here
}

/* system_call_test
 *
 * Trigger system_call_handler by initiating a system call
 * Inputs: None
 * Outputs: Freezed screen with a string showing "system call"/FAIL
 * Side Effects: None
 * Coverage: system call handler
 * Files: idt.h/.c
 */
int system_call_test() {
	TEST_HEADER;
	asm volatile ("int $0x80"); /* initiate a system call */
	return FAIL;
}

/* rtc_test
 *
 * Call test_interrupt() to show that rtc works
 * Inputs: None
 * Outputs: Update the characters shown on the screen at a given frequency/FAIL
 * Side Effects: None
 * Coverage: rtc_handler
 * Files: rtc.h/.c idt.h/.c interrupt_wrapper.S
 */
int rtc_test() {
	TEST_HEADER;
	clear();
	SET_IDT_ENTRY(idt[RTC_VEC], rtc_test_handler_asm); 	// Enable test_interrupt
	while (end_test()); 									// Press enter to end the test
	clear();
	SET_IDT_ENTRY(idt[RTC_VEC], rtc_handler_asm);  		// Disable test_interrupt
	return PASS;
}

/* keyboard_test
 *
 * Echo numbers and characters onto the screen
 * Inputs: None
 * Outputs: Echo numbers and characters onto the screen
 * Side Effects: None
 * Coverage: rtc_handler
 * Files: keyboard.h/.c idt.h/.c interrupt_wrapper.S
 */
int keyboard_test() {
	TEST_HEADER;
	clear();
	while(1);    // Spin to get the input constantly
	return FAIL;
}

/* mem_test
 *
 * Dreference an address to get the content inside
 * Inputs: the address to be tested
 * Outputs: the content at the address/
 *          freezed screen with a string showing the exception
 * Side Effects: None
 * Coverage: paging initialization
 * Files: paging_init.h/.c paging.S
 */
int mem_test(int addr){
	TEST_HEADER;
	uint8_t content = *((uint8_t*)addr); // Dereference the input addr
	printf("content at 0x%x: 0x%x\n", addr, content);
	return PASS;
}

/* kernel_mem_test_vaild
 *
 * Tests for valid addresses for the kernel memory
 * Inputs: None
 * Outputs: the content at the address/
 *          freezed screen with a string showing the exception
 * Side Effects: None
 * Coverage: paging initialization
 * Files: paging_init.h/.c paging.S
 */
int kernel_mem_test_vaild(){
	return mem_test(KENREL_ADDR_1) && mem_test(KENREL_ADDR_2) && mem_test(KENREL_ADDR_3);
}

/* video_mem_test_vaild
 *
 * Tests for valid addresses for the video memory
 * Inputs: None
 * Outputs: the content at the address/
 *          freezed screen with a string showing the exception
 * Side Effects: None
 * Coverage: paging initialization
 * Files: paging_init.h/.c paging.S
 */
int video_mem_test_vaild(){
	return mem_test(VIDEO_ADDR_1) && mem_test(VIDEO_ADDR_2) && mem_test(VIDEO_ADDR_3);
}

/* Checkpoint 2 tests */

/* print_terminal_test()
 *
 * test for the termianl print
 * Inputs: None
 * Outputs: Return PASS
 * Side Effects: print on the screen
 * Coverage: terminal_write terminal_read
 * Files: terminal_drivers.c
 */
int print_terminal_test() {
	TEST_HEADER;
	int i, j;
	int x;
	for (i = 0; i < TERMINAL_PRINT_TEST; i++) {
		for (j = 0; j < i; j++) printf("TEST");
		x = (int)get_cursor_pos() % NUM_COLS;
		if (x != 0) printf("\n");
	}
	return PASS;
}

/* write_terminal_test()
 *
 * test for the terminal_write functionality and also test the handle of a small buffer
 * Inputs: None
 * Outputs: Return PASS
 * Side Effects: None
 * Coverage: terminal_write terminal_read
 * Files: terminal_drivers.c
 */
// int write_terminal_test(){
// 	TEST_HEADER;
// 	int i;
// 	char* input[TERMINAL_WRITE_TEST];
// 	for(i=0;i<TERMINAL_WRITE_TEST;i++){
// 		input[i] = "a";
// 	}
// 	i = terminal_write(0,input,TERMINAL_WRITE_TEST);

// 	return i==TERMINAL_WRITE_TEST ? PASS:FAIL;
// }

/* dir_read_syscall_test
 *
 * Print all the file information in the file system
 * Inputs: None
 * Outputs: Return PASS
 * Side Effects: Print info on the screen
 * Coverage: dir_read
 * Files: file_sys_driver.c
 */
int dir_read_syscall_test(){
	TEST_HEADER;
	uint8_t buf[DIR_BUF];
	uint32_t ret = 0;
	uint32_t len_name, len_len;
	int8_t len_space[LEN_SIZE];
	dentry_t dentry_for_test;
	uint32_t file_len;
	int i;
	while(!ret) {
		ret = dir_read(0, buf, DIR_BUF);
		read_dentry_by_name(buf, &dentry_for_test);
		if (ret) break;
		// Print fname
		printf("file_name: ");
		len_name = strlen((int8_t*)buf);
		for (i = 0; i < NAME_SIZE - len_name; i++) putc(' ');
		printf("%s, ", buf);
		// Print file type
		printf("file_type: %d, ", dentry_for_test.filetype);
		// Print file size
		file_len = inode_start_addr[dentry_for_test.inode_num].length;
		printf("file_size: ");
		itoa(file_len, len_space, TERMINAL_WRITE_TEST); // 10 for decimal
		len_len = strlen(len_space);
		for (i = 0; i < LEN_SIZE - len_len; i++) putc(' ');
		printf("%s\n", len_space);
	}
	return PASS;
}

/* file_read_syscall_test
 *
 * Print the content of a file on the screen
 * Inputs: inode -- dentry index
 * Outputs: Return PASS
 * Side Effects: Print file content on the screen
 * Coverage: file_read
 * Files: file_sys_driver.c
 */
int file_read_syscall_test(int inode){
	TEST_HEADER;
	int8_t buf[FIL_BUF];
	int bytes = file_read(inode, buf, FIL_BUF);
	int i;
	for (i = 0; i < bytes; i++) {
		if (buf[i] != '\0') putc(buf[i]);
	}
	printf("\n");
	printf("# of Bytes Read: %d\n", bytes);
	return PASS;
}

/* dir_read_syscall_test
 *
 * Print limited length of the file content
 * Inputs: inode -- dentry index
 * Outputs: Return PASS
 * Side Effects: Print file content on the screen
 * Coverage: file_read
 * Files: file_sys_driver.c
 */
int file_read_syscall_edge_test(int inode){
	TEST_HEADER;
	int8_t buf[FIL_BUF_EDGE];
	int bytes = file_read(inode, buf, FIL_BUF_EDGE);
	int i;
	for (i = 0; i < bytes; i++) {
		if (buf[i] != '\0') putc(buf[i]);
	}
	printf("\n");
	printf("# of Bytes Read: %d\n", bytes);
	return PASS;
}

/* read_write_terminal_test()
 *
 * echo what user print on the termianl
 * Inputs: None
 * Outputs: Return PASS if b is pressed
 * Side Effects: Echo what is put on the screen when press enter
 * Coverage: terminal_write terminal_read
 * Files: terminal_drivers.c
 */
// int read_write_terminal_test(){
// 	TEST_HEADER;
// 	int i;
// 	int re;
// 	char* output[KEYBOARD_BUFFER_SIZE];
// 	do{
// 		re = terminal_read(0,output,KEYBOARD_BUFFER_SIZE);
// 		for(i=0;i<re;i++){
// 			if (output[i]!=NULL){
// 				printf("%s",output[i]);
// 			}
// 		}
// 		if (i==2){
// 			if (output[0][0]=='b'){
// 				return PASS;
// 			}
// 		}
// 	}while(1);
// 	return i==1? PASS:FAIL;
// }

/* read_write_terminal_test_with_small_buf()
 *
 * echo what user print on the termianl
 * Inputs: None
 * Outputs: Return PASS if b is pressed
 * Side Effects: Echo what is put on the screen when press enter
 * Coverage: terminal_write terminal_read
 * Files: terminal_drivers.c
 */
// int read_write_terminal_test_with_small_buf(){
// 	TEST_HEADER;
// 	int i;
// 	int re;
// 	char* output[FIL_BUF_EDGE];
// 	do{
// 		re = terminal_read(0,output,FIL_BUF_EDGE);
// 		for(i=0;i<re;i++){
// 			if (output[i]!=NULL){
// 				printf("%s",output[i]);
// 			}
// 		}
// 		/*
// 		if (re == FIL_BUF_EDGE){
// 			printf("\n");
// 		}*/
// 		if (i == 2){
// 			if (output[0][0]=='b'){
// 				return PASS;
// 			}
// 		}
// 	}while(1);
// 	return i==1? PASS:FAIL;
// }

/* write_large_terminal_test()
 *
 * input a buffer that is larger
 * Inputs: None
 * Outputs: Return PASS
 * Side Effects: None
 * Coverage: terminal_write
 * Files: terminal_drivers.c
 */
// int write_large_terminal_test(){
// 	TEST_HEADER;
// 	int i;
// 	char* input[KEYBOARD_BUFFER_SIZE+10];  // +10 for a larger buffer size
// 	for (i=0;i<KEYBOARD_BUFFER_SIZE+10;i++){
// 		input[i]="a";
// 	}
// 	i = terminal_write(0,input,KEYBOARD_BUFFER_SIZE+10);
// 	return i==127? PASS:FAIL;
// }

/* rtc_valid_freq_test
 *
 * check if rtc_write successfully rule out invalid input
 * Inputs: None
 * Outputs: Return PASS if all desired return values received
 * Side Effects: None
 * Coverage: rtc_open rtc_close rtc_read rtc_write
 * Files: rtc.c
 */
int rtc_valid_freq_test(){
	TEST_HEADER;
	int freq[RTC_TEST_LENGTH] = {RTC_INVALID_1, RTC_INVALID_2, RTC_INVALID_3, RTC_VALID_1, RTC_VALID_2};
	int ret[RTC_TEST_LENGTH];
	int i;
	(void)rtc_open(0);
	(void)rtc_read(0,0,0);
	for (i = 0; i < RTC_TEST_LENGTH; i++){
		printf("Writing fequency %dHz to rtc...", freq[i]);
		if ((ret[i] = rtc_write(0,freq + i, sizeof(int)))){
			printf("Fail\n");
		} else {
			printf("Success\n");
		}
	}
	(void)rtc_close(0);
	if(ret[0] == -1 && ret[1] == -1 && ret[2] == -1 && ret[3] == 0 && ret[4] == 0){
		return PASS;
	}
	return FAIL;
}

/* rtc_read_write_test
 *
 * print characters according to rtc frequency
 * Inputs: None
 * Outputs: Return PASS
 * Side Effects: Print characters to the terminal according to rtc freq
 * Coverage:  rtc_open rtc_close rtc_read rtc_write
 * Files: rtc.c
 */
int rtc_read_write_test(){
	TEST_HEADER;
	int freq;
	int i, j;
	(void)rtc_open(0);
	for (i = 0; i < RTC_TEST_NUM; i++){
		freq = INITIAL_FREQ << i;
		printf("Writing new frequency %dHz...\n", freq);
		(void)rtc_write(0,&freq, sizeof(int));
		for (j = 0; j < freq * RTC_TEST_LENGTH; j++){
			(void)rtc_read(0,0,0);
			printf("1");
		}
		clear();
	}
	(void)rtc_close(0);
	return PASS;
}
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */

/* Test suite entry point
 * Uncomment one test at a time to check for the functionalities.
 *
 */
void launch_tests(){
/* Checkpoint 1 tests */
	/* Check whether the entries of idt is empty */
	// TEST_OUTPUT("idt_test", idt_test());

	/* Tests to trigger different exceptions and system call*/
	// TEST_OUTPUT("DE_test", DE_test());
	// TEST_OUTPUT("OF_test", OF_test());
	// TEST_OUTPUT("UD_test", UD_test());
	// TEST_OUTPUT("PF_test", PF_test());
	// TEST_OUTPUT("system_call_test", system_call_test());

	/* Test for rtc */
  	// TEST_OUTPUT("rtc_test", rtc_test());
	/* Test for keyboard */
	// TEST_OUTPUT("keyboard_test", keyboard_test());

	/* Tests for paging */
	// TEST_OUTPUT("kernel_mem_test_before_start", mem_test(KENREL_ADDR_4)); // PF exception
	// TEST_OUTPUT("kernel_mem_test_vaild", kernel_mem_test_vaild()); // Pass
	// TEST_OUTPUT("kernel_mem_test_after_end", mem_test(KENREL_ADDR_5)); // PF exception
	// TEST_OUTPUT("video_mem_test_before_start", mem_test(VIDEO_ADDR_4)); // PF exception
	// TEST_OUTPUT("video_mem_test_vaild", video_mem_test_vaild()); // Pass
	// TEST_OUTPUT("video_mem_test_after_end", mem_test(VIDEO_ADDR_5)); // PF exception

/* Checkpoint 2 tests */
	// Test for rtc read/write
	// test_wrapper_no_param("rtc_valid_freq_test", rtc_valid_freq_test);
	// test_wrapper_no_param("rtc_read_write_test", rtc_read_write_test);

	// Test for Terminal
	// test_wrapper_no_param("terminal print test", print_terminal_test);
	// test_wrapper_no_param("terminal write test", write_terminal_test);  // test for write and also a case that input buffer is smaller
	// test_wrapper_no_param("terminal read write test", read_write_terminal_test); // the test for both the func of read and wirte
	// test_wrapper_no_param("write a large buffer to terminal test", write_large_terminal_test); // write a buffer larger than the key buffer
	// test_wrapper_no_param("read write with samll buffer test", read_write_terminal_test_with_small_buf); // read with a samll buffer

	// Test for file
	// test_wrapper_no_param("read directory test",  dir_read_syscall_test);
	// test_wrapper_int("read file test: executable 1", file_read_syscall_test, GREP); // Test print excutable 1
	// test_wrapper_int("read file test: executable 2", file_read_syscall_test, LS); // Test print excutable 2
	// test_wrapper_int("read file test: large 1", file_read_syscall_test, FISH); // Test print large 1
	// test_wrapper_int("read file test: large 2", file_read_syscall_test, VERYLARGE); // Test print large 2
	// test_wrapper_int("read file test: small 1", file_read_syscall_test, FRAME0); // Test print small 2
	// test_wrapper_int("read file test: small 2", file_read_syscall_test, FRAME1); // Test print small 2
	// test_wrapper_int("read file test edge case: small buffer", file_read_syscall_edge_test, FRAME0); // Test small buffer

	// End testing
	printf("All tests executed.");
}

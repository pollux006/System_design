/* keyboard.h - Defines used in initialize keyboards
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

// Magic numbers defines for keyboard initialization
#define KEYBOARD_IRQ 1
#define KEYBOARD_PORT 0x60
#define RELEASE_CHECK 0x80
#define KEYBOARD_BUFFER_SIZE 128
#define KEY_MASK 0x0FF
#define KEY_NEED_IMPLEMENT 0x3A
//special key defined here
//key extent
#define KEY_EXTENT  0xE0
//shift
#define L_SHIFT_PRESESSED 0x2A
#define R_SHIFT_PRESESSED 0x36
#define L_SHIFT_RELEASED 0xAA
#define R_SHIFT_RELEASED 0xB6
//control
#define L_CTRL_PRESESSED 0x1D
#define R_CTRL_PRESESSED 0xE0
#define L_CTRL_RELEASED 0x9D
#define R_CTRL_RELEASED 0x9D
//alt
#define L_ALT_PRESESSED 0x38
#define L_ALT_RELEASED 0xB8
//CPASLOCK
#define CAPSLOCK_PRESESSED 0x3A
//CURSOR
#define CURSOR_L  0x4B
#define CURSOR_R  0x4D
#define CURSOR_U  0x48
#define CURSOR_D  0x50
//BackSpace
#define BACKSPACE 0x0E
// key l
#define KEY_L 0x26
#define KEY_C 0x2E
// key enter
#define KEY_ENTER 0x1C
// key tab
#define KEY_TAB 0x0F
// define start and end of some letter
#define Q_KEY 0x10
#define P_KEY 0x19
#define A_KEY 0x1E
#define L_KEY 0x26
#define Z_KEY 0x2C
#define M_KEY 0x32
#define F1_KEY	0x3B
#define F2_KEY	0x3C
#define F3_KEY	0x3D

// multiterminal related
#define TER_NUM 3   // three terminals

// Define the look-up table according to the scancode and corresponding key
extern char key_array[2][58]; // 58 is the 0x36 the last key we need
extern char key_buffer[TER_NUM][KEYBOARD_BUFFER_SIZE];
volatile int buf_status[TER_NUM];

// Enable keyboard interrrupt on PIC
void keyboard_init();
// clear keyboard buffer
void key_buf_clear(unsigned int ter_num);
//clear all the buffer
void clear_all_buf();
// Define the keyboard interrupt handler
void keyboard_handler();
int end_test();

//check letter key
int is_letter(unsigned int key);
//handle special key
int handle_special(unsigned int key);
//check buffer full or not
int buf_check_full();
//check buffer empty or not
int buf_check_empty();
//delete the char before cursor
void delete_bf_cursor();
//add the char at cursor
void add_at_cursor(char content);
//reprint buf
void reprint_buf();
//move_cursor left or right by a char
void move_cursor(int dir);
//clear the terminal  used when scheduling is enabled
void clear_terminal_scheduling();

// Mark the scancode volatile
volatile unsigned int key;

#endif

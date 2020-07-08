#include "types.h"
#include "keyboard.h"
#include "task.h"

#define CURSOR_START 0
#define CURSOR_END 15
#define VIDEO_PORT 0x3D4
#define VIDEO_DATA 0x3D5
#define CURSOR_START_REG 0x0A
#define CURSOR_END_REG 0x0B
#define CURSOR_LOC_HIGH_REG 0xE
#define CURSOR_LOC_LOW_REG 0xF

#define CURSOR_DISABLE_BITS 0x20
#define REG_MASK 0xFF
#define HIGH_SHIFT 8

#define NUM_COLS    80
#define NUM_ROWS    25
#define VIDEO       0xB8000
#define ATTRIB      0x7
#define LAST_ROW    (NUM_ROWS - 1)

uint16_t pos_buf[TER_NUM];
int screen_x_buf[TER_NUM];
int screen_y_buf[TER_NUM];  // two value for tracking screen value
/* move the cursor */
void enable_cursor();
void disable_cursor();
void update_cursor(int x, int y);
void update_cursor_pos(uint16_t pos);
uint16_t get_cursor_pos();
void update_cursor_ter(uint16_t pos);

/* scroll up the screen */
void scroll_up();

/* clear the terminal */
void clear_terminal();
/*switch the video */
void switch_video(unsigned int ter);
/*switch the cursor and screen location when switch process*/
void switch_process_cursor(uint8_t process_ter, uint8_t next_ter);
void update_screen_buf(uint8_t ter);

/* Terminal System Calls */
int32_t terminal_open(const int8_t* fname);
int32_t terminal_close(int32_t fd);
int32_t terminal_read(int32_t fd, void* buf, int32_t length);
int32_t terminal_write(int32_t fd, const void* buf, int32_t length);

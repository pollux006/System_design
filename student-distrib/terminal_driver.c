#include "terminal_driver.h"
#include "lib.h"
#include "paging_init.h"
#include "x86_desc.h"
#include "pcb.h"
#include "system_call.h"

static char* video_mem = (char *)VIDEO;
// static unsigned int cur_ter = 0;
// static unsigned int cursor_pos[TER_NUM];


/* enable_cursor()
 *
 * enable cursor to display on screen
 * Inputs: none
 * Outputs: None
 * Side Effects: enable cursor to display on screen
 */
void enable_cursor(){
    uint8_t cs_reg;    // The content of Cursor Start Register
    uint8_t new_cs_reg;    // The content of Cursor Start Register
    uint8_t ce_reg;    // The content of Cursor End Register
    uint8_t new_ce_reg;    // The content of Cursor End Register

    outb(CURSOR_START_REG, VIDEO_PORT);
    cs_reg = inb(VIDEO_DATA);
    new_cs_reg = (cs_reg & 0xC0) | CURSOR_START; // Set bit 0-5 of Cursor Start register
                                                 // 0xC is the bit mask for preserving bit 6-7
	outb(new_cs_reg, VIDEO_DATA);

	outb(CURSOR_END_REG, VIDEO_PORT);
    ce_reg = inb(VIDEO_DATA);
    new_ce_reg = (ce_reg & 0xE0) | CURSOR_END; // Set bit 0-4 of Cursor Start register
                                               // 0xE is the bit mask for preserving bit 5-7
	outb(new_ce_reg, VIDEO_DATA);
}

/* disable_cursor()
 *
 * stop cursor to be displayed on screen
 * Inputs: none
 * Outputs: None
 * Side Effects: disable cursor to display on screen
 */
void disable_cursor(){
    outb(CURSOR_START_REG, VIDEO_PORT);
    outb(CURSOR_DISABLE_BITS, VIDEO_DATA);
}

/* update_cursor(int x, int y)
 *
 * update the cursor location on the screen
 * Inputs: int x --> the x location of new cursor
 *         int y --> the y location of new cursor
 * Outputs: None
 * Side Effects: changes the location of the cursor
 */
void update_cursor(int x, int y){
    uint16_t pos = y * NUM_COLS + x;

    outb(CURSOR_LOC_LOW_REG, VIDEO_PORT);
    outb((uint8_t)(pos & REG_MASK), VIDEO_DATA);
    outb(CURSOR_LOC_HIGH_REG, VIDEO_PORT);
    outb((uint8_t)((pos >> HIGH_SHIFT) & REG_MASK), VIDEO_DATA);
}

/* update_cursor(uint16_t pos)
 *
 * update the cursor location on the screen
 * Inputs:  uint16_t -> the postion of the new cursor  (pos = y * NUM_COLS + x)
 * Outputs: None
 * Side Effects: changes the location of the cursor
 */
void update_cursor_pos(uint16_t pos){
    outb(CURSOR_LOC_LOW_REG, VIDEO_PORT);
    outb((uint8_t)(pos & REG_MASK), VIDEO_DATA);
    outb(CURSOR_LOC_HIGH_REG, VIDEO_PORT);
    outb((uint8_t)((pos >> HIGH_SHIFT) & REG_MASK), VIDEO_DATA);
}

/* update_cursor_ter(uint16_t pos)
 *
 * update the cursor location on the screen
 * Inputs:  uint16_t -> the postion of the new cursor  (pos = y * NUM_COLS + x)
 * Outputs: None
 * Side Effects: changes the location of the cursor
 */
void update_cursor_ter(uint16_t pos){
    pcb_t* cur_pcb = get_pcb_by_id(active_process);
    if(cur_pcb==0){update_cursor_pos(pos);}
    pos_buf[cur_pcb->terminal] = pos;
    if(cur_pcb->terminal == active_ter){
        update_cursor_pos(pos);
    }
}

/* get_cursor_pos()
 *
 * get the cursor location on the screen
 * Inputs:  None
 * Outputs: uint16_t cursor location on the screen
 * Side Effects: None
 */
uint16_t get_cursor_pos()
{
    uint16_t pos = 0;  // Clear bits of pos
    uint8_t low_byte;
    uint16_t high_byte;

    outb(CURSOR_LOC_LOW_REG, VIDEO_PORT);
    low_byte = inb(VIDEO_DATA);
    pos |= low_byte;

    outb(CURSOR_LOC_HIGH_REG, VIDEO_PORT);
    high_byte = ((uint16_t)inb(VIDEO_DATA)) << HIGH_SHIFT;
    pos |= high_byte;

    return pos;
}

/* scroll_up()
 *
 * scroll_up the whole screen by 1
 * Inputs:  None
 * Outputs: None
 * Side Effects: Make the display of the screen up by 1
 */
void scroll_up(){
    int i; // Loop counter
    for (i = 0; i < LAST_ROW * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = *(uint8_t *)(video_mem + ((NUM_COLS + i) << 1));
        *(uint8_t *)(video_mem + (i << 1) + 1) = *(uint8_t *)(video_mem + ((NUM_COLS + i) << 1) + 1);
    }

    for (i = LAST_ROW * NUM_COLS; i < NUM_COLS * NUM_ROWS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
}

/* clear_terminal()
 *
 *  clear the terminal
 * Inputs:  None
 * Outputs: None
 * Side Effects: clear all the thing on screen and move the cursor to left top
 */
void clear_terminal(){
    clear();
}

/* terminal_open()
 *
 * open up the terminal
 * Inputs:  None
 * Outputs: 0 for success
 * Side Effects: enable the cursor and clear the screen
 */
int32_t terminal_open(const int8_t* fname){
    unsigned int i;
    enable_cursor();
    // clear the cusor buf
    for(i=0;i<TER_NUM;i++){
        pos_buf[i]=0;
        screen_x_buf[i]=0;
        screen_y_buf[i]=0;
        pos_buf[i]=0;
    }
    clear_terminal();

    return 0;
}

/* terminal_close()
 *
 * close the terminal
 * Inputs:  None
 * Outputs: 0 for success
 * Side Effects: enable the cursor and clear the screen
 */
int32_t terminal_close(int32_t fd){
    clear();   //clear the terminal
    return 0;
}

/* terminal_read(char* buf,unsigned int length)
 *
 *  start to read from terminal and returns what is type after enter key pressed
 * Inputs:  char** buf -> the buffer used to store the output from keyboard
 *          unsigned int length -> the length of the input buffer
 * Outputs: int -> the readed byte
 * Side Effects: None
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t length){
    if (!buf) return -1;  // if buf is null, return -1
    int count;  // readed byte from keyboard buffer
    char* buf_read = (char*)buf;
    int i;
    count=0;
    pcb_t* cur_pcb = get_pcb_by_id(active_process);
    uint8_t process_ter = cur_pcb->terminal;
    if(buf_status[process_ter]!=1){    // if the previous buffer is closed, open it
        key_buf_clear(process_ter);
        buf_status[process_ter]=1;
    }
    while(buf_status[process_ter]==1);   // wait the user to input
    printf("\n");
    cli();
    for(i=0; i<length; i++){      // write to buffer
        if(i>=KEYBOARD_BUFFER_SIZE){
            buf_read[i] = NULL;
            continue;
        } else if(key_buffer[process_ter][i] == NULL){
            break;
        } else{
            buf_read[i] = key_buffer[process_ter][i];
            count++;
        }
    }
    if (i == length){
      buf_read[i - 1] = '\n';
    }
    sti();
    return count;
}

/* terminal_write(char* buf,unsigned int length)
 *
 *  start to write to keyboard buffer
 * Inputs:  char** buf -> the buffer used to write to keyboard buffer
 *          unsigned int length -> the length of the input buffer
 * Outputs: int -> the writed byte
 * Side Effects: None
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t length){
    cli();
    if (!buf) return -1;  // if buf is null, return -1
    int count = 0;  // writed byte to keyboard buffer
    // int8_t* buf_write = (int8_t*) buf;
    int i;
    for(i=0;i<length;i++){
        // if(((int8_t*)buf)[i]==NULL){continue;}
        printf("%c",((int8_t*)buf)[i]);
        count++;
        update_cursor_ter(get_y()*NUM_COLS+get_x()); /*fix*/
    }
    sti();
    return count;
}

/* void switch_video(unsigned int ter)
 *
 *  switch the video memory of the displyed and also save to backup
 * Inputs:  usigned int ter  -> the terminal switched to
 * Outputs: none
 * Side Effects: switch the display to another terminal and also save the current terminal video to backup
 */
void switch_video(unsigned int ter){
    cli();
    if(ter<0 || ter>=TER_NUM) return;
    video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF;
    flushTLB();
    int8_t* tar = (int8_t*)(VIDEO + (ter+1)* VIDEO_SIZE);
    int8_t* cur = (int8_t*)(VIDEO + (active_ter+1)*VIDEO_SIZE);
    // we copy the current terminal to the buffer
    memcpy(cur,(int8_t*)VIDEO,VIDEO_SIZE);
    pos_buf[active_ter] = get_cursor_pos();
    // copy the target terminal
    memcpy((int8_t*)VIDEO,tar,VIDEO_SIZE);
    update_cursor_pos(pos_buf[ter]);
    active_ter = ter;
    pcb_t* active_pcb = get_pcb_by_id(active_process);
    uint8_t restore_ter = active_pcb -> terminal;
    video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF + restore_ter + 1;
    flushTLB();
    return;
}

/*fix*/
/* void switch_process_cursor(uint8_t process_ter, uint8_t next_ter)
 *
 *  switch the screen related when switching process
 * 
 * Inputs:  uint8_t process_ter -> current process's terminal
 *           uint8_t next_ter   -> terminal switch to
 * Outputs: none
 * Side Effects: switch the screen_x and screen_y value
 */
void switch_process_cursor(uint8_t process_ter, uint8_t next_ter){
    update_screen_buf(process_ter);
    update_x_y(screen_x_buf[next_ter],screen_y_buf[next_ter]);
}

/* void update_screen_buf(uint8_t ter)
 *
 *  update the screen-buffer by current screen value
 * 
 * Inputs:  ter -> the buffer that need update
 * Outputs: none
 * Side Effects: changes screen buffer
 */
void update_screen_buf(uint8_t ter){
    screen_x_buf[ter] = get_x();
    screen_y_buf[ter] = get_y();
}

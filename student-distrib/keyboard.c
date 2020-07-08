/* keyboard.c - Functions used in initialize keyboards
 */

#include "keyboard.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "terminal_driver.h"
#include "system_call.h"
#include "pcb.h"
#include "paging_init.h"

#include "signal.h"

// four variables for recording if a special key pressed
char ctrl;
char shift;
char capslock;
char alt;

// buffer of the key
char key_buffer[TER_NUM][KEYBOARD_BUFFER_SIZE];
// keep track of position in buffer for cursor use
static int buffer_pos[TER_NUM];
// the current terminal
static unsigned int cur_ter;

// Define the look-up table according to the scancode and corresponding key
char key_array[2][58] = {{   // 58 is 0x3A (see right)
    0,0,'1','2','3','4','5','6','7','8','9','0',                // 0x00 - 0x0B
    '-','=',0/*'backspace'*/,0/*'tab'*/,'q','w','e','r','t','y','u',              // 0x0C - 0x16
    'i','o','p','[',']','\n',0/*'left control'*/,'a','s','d','f',         // 0x17 - 0x21
    'g','h','j','k','l',';','\'',0/*'back tick'*/,0/*'left shift'*/,    // 0x22 - 0x2A
    '\\','z','x','c','v','b','n','m',',','.','/',0/*'right shift'*/,         // 0x2B - 0x36
    '*',0/*"left alt"*/,' '
},{                                                             // for keys that are in shift status
    0,0/*'escape'*/,'!','@','#','$','%','^','&','*','(',')',                // 0x00 - 0x0B
    '_','+',0/*'backspace'*/,0/*'tab'*/,'Q','W','E','R','T','Y','U',              // 0x0C - 0x16
    'I','O','P','{','}','\n',0/*'left control'*/,'A','S','D','F',         // 0x17 - 0x21
    'G','H','J','K','L',':','\'',0/*'back tick'*/,0/*'left shift'*/,    // 0x22 - 0x2A
    '|','Z','X','C','V','B','N','M','<','>','?',0/*'right shift'*/,         // 0x2B - 0x36
    '*',0/*'left alt'*/,' '                                                 // 0x54 - 0x58
}};

/* keyboard_init
 *
 * Enable keyboard interrrupt on PIC
 * Inputs: None
 * Outputs: None
 * Side Effects: enable keyboard interrupt
 */
void keyboard_init(){
    int i;
    for (i = 0; i<TER_NUM;i++){
        buf_status[i] = 0; //disable buffer at first
        buffer_pos[i] = 0;
    }
    
    cur_ter =0;
    clear_all_buf();  // clear buffer
    shift = 0;
    alt =0;
    ctrl = 0;
    capslock = 0;  // clear special key status
    enable_irq(KEYBOARD_IRQ);  // Enable interrrupts coming in
}

/* clear_all_buf
 *
 * clear keyboard buffer
 * Inputs: None
 * Outputs: None
 * Side Effects: clear all keyboard buffer
 */
void clear_all_buf(){
    int i,j;
     for (i = 0; i<TER_NUM;i++){
        buffer_pos[i] = 0;  //reset buffer position
        for(j = 0;j<KEYBOARD_BUFFER_SIZE;j++){
            key_buffer[i][j]=NULL;
        }
    }
    
}

/* key_buf_clear
 *
 * clear keyboard buffer
 * Inputs: None
 * Outputs: None
 * Side Effects: clear keyboard buffer
 */
void key_buf_clear(unsigned int ter_num){
    int j;
    buffer_pos[ter_num] = 0;  //reset buffer position
    for(j = 0;j<KEYBOARD_BUFFER_SIZE;j++){
         key_buffer[ter_num][j]=NULL;
    }    
}

/* is_letter(unsigned int key)
 *
 * check if it is a letter key
 * Inputs: key -- the pressed key num
 * Outputs: 1 if it is a letter key, 0 not
 * Side Effects: None
 */
int is_letter(unsigned int key){
    if (key>=Q_KEY && key<=P_KEY){return 1;}
    if (key>=A_KEY && key<=L_KEY){return 1;}
    if (key>=Z_KEY && key<=M_KEY){return 1;}
    return 0;
}

/* handle_special
 *
 * handle speical keyoard input
 * Inputs: key -- the pressed key num
 * Outputs: 1 if it is a special key, 0 not
 * Side Effects: update special key status
 */
int handle_special(unsigned int key){
    // printf("get key:%x\n",key);
    switch (key)   //
    {
    case L_SHIFT_PRESESSED:
        // printf("shift pre");
        shift =1;
        return 1;
    case R_SHIFT_PRESESSED:
        shift =1;
        return 1;
    case L_SHIFT_RELEASED:
        // printf("shift re");
        shift =0;
        return 1;
    case R_SHIFT_RELEASED:
        shift =0;
        return 1;
    case L_CTRL_PRESESSED:
        ctrl =1;
        return 1;
    case L_CTRL_RELEASED:
        ctrl =0;
        return 1;
    case L_ALT_PRESESSED:
        alt =1;
        return 1;
    case L_ALT_RELEASED:
        alt =0;
        return 1;
    case CAPSLOCK_PRESESSED:
        capslock = capslock? 0:1;   // switch state according to previous state
        return 1;
    default:
        return 0;
    }
}

/* keyboard_handler
 *
 * Define the keyboard interrupt handler
 * Inputs: None
 * Outputs: None
 * Side Effects: handle keyboard interrupt
 */
void keyboard_handler(){
    cli();
    int special = 0; // flag of sepcial key
    int i;
    // Get keyboard scancode
    key = (unsigned int)inb(KEYBOARD_PORT);

    // handle special key
    special  =  handle_special(key);
    // check special key
    if (special == 1){
        send_eoi(KEYBOARD_IRQ);
    }else{
    // normal key
        // switch terminal
        if (alt == 1 && (key >= F1_KEY && key<= F3_KEY)){
            send_eoi(KEYBOARD_IRQ);
            unsigned int next_ter  = key-F1_KEY; 
            // shift the terminal if not same terminal
            if(cur_ter!=next_ter){
                cur_ter = next_ter;
                switch_video(next_ter);
                /*fix*/
            }
        sti();
        return;
        // check if is ctrl+l
        }else if (ctrl == 1 && key == KEY_L) {
                clear_terminal_scheduling();
                update_cursor_pos(0); // set the cursor back
                /*fix*/
                screen_x_buf[cur_ter] =0;
                screen_y_buf[cur_ter] =0;
        }
        
        #ifdef TEST_EXTRA        
        else if(ctrl == 1 && key == KEY_C){
            // Ctrl + c, generate INTERRUPT signal
            send_eoi(KEYBOARD_IRQ);
            //exception_halt();
            printf("KeyboardInterrupt\n");
            signal_generate(INTERRUPT);
        #endif

        #ifndef TEST_EXTRA
        else if(ctrl == 1 && key == KEY_C){
            /*fix*/
            printf("KeyboardInterrupt\n");
            send_eoi(KEYBOARD_IRQ);
            keyboard_halt(active_ter);
        #endif

        }else if(key == CURSOR_L){
            move_cursor(0);
        }else if(key == CURSOR_R){
            move_cursor(1);
        }
        else if (key == BACKSPACE ){
            if(!buf_check_empty() && buf_status[cur_ter]){      // do not write && for situation that pressed backspace but empty
                delete_bf_cursor();
            }
        }
        // if reach max buf
        else if (buf_check_full() && key != KEY_ENTER){
            /*do nothing*/
        }
        // if enter pressed
        else if (key == KEY_ENTER){
        //    add_at_cursor('\n');
           for(i=0;i<KEYBOARD_BUFFER_SIZE;i++){
               if(key_buffer[cur_ter][i]==NULL){
                   key_buffer[cur_ter][i]='\n';
                   break;
               }
           }
           buf_status[cur_ter]=0; // set buffer not avaliable to able read to buffer
        }
        // Check the interrupt is a key pressed or released
        else if((key & RELEASE_CHECK) != NULL){
            /*do nothing*/
        }
        else
        {
            if (key == KEY_TAB){
                add_at_cursor(' ');
                add_at_cursor(' ');
                add_at_cursor(' ');
                add_at_cursor(' ');   // add four space
            }
            else if(key<KEY_NEED_IMPLEMENT){
                if(is_letter(key)){
                    add_at_cursor(key_array[capslock^shift][key]);
                }
                else{
                    add_at_cursor(key_array[(int)shift][key]);
                }
            }
        }
        // Send EOI after interrupt is done
        send_eoi(KEYBOARD_IRQ);
    }
    sti();

}

/* end_test()
 *
 * Check if the input key is enter
 * Inputs: None
 * Outputs: whether the key is enter or not
 * Side Effects: None
 */
int end_test(){
    return (key_array[0][key] != '\n');
}


/* buf_check_full
 *
 * check buffer full or not
 * Inputs: none
 * Outputs: 1 if it is full, 0 not
 * Side Effects: none
 */
int buf_check_full(){
    if (key_buffer[cur_ter][KEYBOARD_BUFFER_SIZE-2] != NULL){ // 2 is for the second last buf since last must be enter
        return 1;
    }
    return 0;
}


/* buf_check_empty
 *
 * check buffer empty or not
 * Inputs: none
 * Outputs: 1 if it is empty, 0 not
 * Side Effects: none
 */
int buf_check_empty(){
    if (key_buffer[cur_ter][0] == NULL){ // check the first char in buf
        return 1;
    }
    return 0;
}

/* move_cursor
 *
 * move_cursor left or right by a char
 * Inputs: dir  0 for left and 1 for right
 * Outputs: none
 * Side Effects: move the cursor
 */
void move_cursor(int dir){
    if(dir==0){
        if(buffer_pos[cur_ter]==0){
            return;  //do nothing if at leftmost
        }
        else{
            buffer_pos[cur_ter]--;
            update_cursor_pos(get_cursor_pos()-1);
        }
    }
    else{
        // KEYBOARD_BUFFER_SIZE-2 is the last buffer position before enter
         if(buffer_pos[cur_ter]==KEYBOARD_BUFFER_SIZE-2 || key_buffer[cur_ter][buffer_pos[cur_ter]] == NULL){
            return;  //do nothing if at leftmost
        }
        else{
            buffer_pos[cur_ter]++;
            update_cursor_pos(get_cursor_pos()+1);
        }
    }
    screen_x_buf[cur_ter] = get_cursor_pos()%NUM_COLS;
    screen_y_buf[cur_ter] = get_cursor_pos()/NUM_COLS;
    /*fix*/
}

/* add_at_cursor
 *
 * add the char at cursor
 * Inputs: none
 * Outputs: none
 * Side Effects: add the char at cursor and reprint buffer
 */
void add_at_cursor(char content){
    unsigned int i;
    unsigned int moved;
    char temp1;
    char temp2;
    moved =0;
    /*fix*/
    cli();
    pcb_t* cur_pcb = get_pcb_by_id(active_process);
    uint8_t process_ter = cur_pcb->terminal;
    if(process_ter!=cur_ter){
        //switch paging and screen value
        video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF;
        flushTLB();
        update_screen_buf(process_ter);
        update_x_y(screen_x_buf[cur_ter],screen_y_buf[cur_ter]);
    }
    if (buf_status[cur_ter]!=1){
        buf_status[cur_ter]=1;
        key_buf_clear(cur_ter);
    }
    move_screen_to_cursor_position(); //set the next printing char pos to cursor
    if(key_buffer[cur_ter][buffer_pos[cur_ter]]==NULL){  // case that cursor is at the end of a line
        key_buffer[cur_ter][buffer_pos[cur_ter]] = content;
        printf("%c",content);
        moved++;
    }else{
        temp2 = content;
        // go through the buffer and move all char back by 1 and print
        for(i=buffer_pos[cur_ter];i<KEYBOARD_BUFFER_SIZE;i++){
            temp1 = temp2;
            temp2 = key_buffer[cur_ter][i];
            key_buffer[cur_ter][i] = temp1;
            printf("%c",temp1);
            //update_cursor(get_x(),get_y()); /*fix*/
            moved++;
            if(temp2 ==NULL){break;}
        }
    }
    /*fix*/
    //+1 is for move left for one char after insert  moved is for curso not at end  getx+gety*num is screen location
    update_cursor_pos(get_x()+get_y()*NUM_COLS-moved+1);  
    buffer_pos[cur_ter]++;
    if(process_ter!=cur_ter){
        //switch paging back
        video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF + process_ter + 1;
        flushTLB();
        update_x_y(screen_x_buf[process_ter],screen_y_buf[process_ter]);
    }
}

/* delete_bf_cursor
 *
 * delete the char before cursor
 * Inputs: none
 * Outputs: none
 * Side Effects: delete the char before cursor and reprint buffer
 */
void delete_bf_cursor(){
    cli();
    pcb_t* cur_pcb = get_pcb_by_id(active_process);
    uint8_t process_ter = cur_pcb->terminal;
    if(process_ter!=cur_ter){
        //switch paging and screen value
        video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF;
        flushTLB();
        update_screen_buf(process_ter);
        update_x_y(screen_x_buf[cur_ter],screen_y_buf[cur_ter]);
    }
    unsigned int i;
    int16_t pos;
    pos = get_cursor_pos();
    if(buffer_pos[cur_ter]==0){return;} //do nothing if cursor is at the start
    update_cursor_pos(pos-1); // delete from the char before cursor
    buffer_pos[cur_ter]--;
    move_screen_to_cursor_position(); //set the next printing char pos to cursor
    // go through the buffer and move all char back by 1 and print
    for(i=buffer_pos[cur_ter];i<KEYBOARD_BUFFER_SIZE-1;i++){
        key_buffer[cur_ter][i] = key_buffer[cur_ter][i+1];
        if(key_buffer[cur_ter][i] == NULL){
            printf("%c",' ');  // print a space to overlap the last char
            break;
            }
        printf("%c",key_buffer[cur_ter][i]);
    }
    if(process_ter!=cur_ter){
        //switch paging back
        video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF + process_ter + 1;
        flushTLB();
        update_x_y(screen_x_buf[process_ter],screen_y_buf[process_ter]);
    }
}


/* reprint_buf
 *
 * reprint the current line by the buffer
 * Inputs: none
 * Outputs: none
 * Side Effects: reprints buffer content form the current cursor position
 * and set the cursor to end
 */
void reprint_buf(){
    unsigned int i;
    //print all things in buffer
    for(i=0;i<KEYBOARD_BUFFER_SIZE;i++){
        if (key_buffer[i]==NULL){
            buffer_pos[cur_ter] = i;
            break;   // this is to overlap the char before on the screen
        }
        else{
            printf("%c",key_buffer[cur_ter][i]);
        }
    }
    // update_cursor_pos(get_cursor_pos()+buffer_pos-KEYBOARD_BUFFER_SIZE);
}

/* clear_terminal_scheduling()
 *
 * clear the terminal  used when scheduling is enabled
 * Inputs:  None
 * Outputs: None
 * Side Effects: clear all the thing on screen and move the cursor to left top   might switch page
 */
void clear_terminal_scheduling(){
    pcb_t* cur_pcb = get_pcb_by_id(active_process);
    uint8_t process_ter = cur_pcb->terminal;
    if(process_ter!=cur_ter){
        //switch paging and screen value
        video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF;
        flushTLB();
        update_screen_buf(process_ter);
        update_x_y(screen_x_buf[cur_ter],screen_y_buf[cur_ter]);
    }
    clear();
    if(process_ter!=cur_ter){
        //switch paging back
        video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF + process_ter + 1;
        flushTLB();
        update_x_y(screen_x_buf[process_ter],screen_y_buf[process_ter]);
    }

}

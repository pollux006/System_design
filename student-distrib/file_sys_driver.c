#include "file_sys_driver.h"
#include "terminal_driver.h"
#include "pcb.h"
#include "rtc.h"

static int dentry_num;      // Total number of directory entries
static int inode_num;       // Total number of index nodes
static int d_block_num;     // Total number of data block numbers
static uint32_t dir_idx;    // Current directory index for dir_read

/* init_fs
 *
 * Initialize the file system by setting the constants
 * used in file structures.
 * Inputs: None
 * Outputs: None
 * Side Effects: Set global variables for constants.
 */
void init_fs(uint32_t fs_addr_start){
    b_block_addr = (file_boot_block_t*) fs_addr_start;
    dentry_num = b_block_addr -> dir_cnt;
    inode_num = b_block_addr -> inode_cnt;
    d_block_num = b_block_addr -> data_cnt;
    dentry_addr = b_block_addr -> dir_entries;
    inode_start_addr = (inode_t*) &(b_block_addr[BOOT_BLOCK_NUM]);
    d_block_start_addr = (data_block_t*) &(inode_start_addr[inode_num]);
    init_fop_table();
}

/* read_dentry_by_index
 *
 * Helper funtion that takes in dentry index and saves the dentry
 * data to a in put buffer.
 * Inputs: index -- index of the dentry that wants to read
 *         dentry -- saves the content of the wanted data
 * Outputs: Return 0 for success
 *          Return -1 for index is invalid
 * Side Effects: Save the value of target into dentry
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
    if (index >= dentry_num) return -1;
    memcpy(dentry, &(dentry_addr[index]), DENTRY_SIZE);
    return 0;
}

/* read_dentry_by_index
 *
 * Helper funtion that takes in inode name and saves the dentry
 * data to a in put buffer.
 * Inputs: fname -- the name of the target file
 *         dentry -- saves the content of the wanted data
 * Outputs: Return 0 for success
 *          Return -1 for cannot find the name
 * Side Effects: Save the value of target into dentry
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
    int32_t index;
    int8_t* cur_fname;
    int32_t f_len = strlen((int8_t*) fname);
    for (index = 0; index < inode_num; index ++){
        cur_fname = dentry_addr[index].filename;
        int32_t cur_len = strlen((int8_t*) cur_fname);
        if (cur_len > NAME_LEN) cur_len = NAME_LEN;
        int32_t len = (cur_len > f_len) ? cur_len : f_len;

        if (!strncmp((int8_t*) fname, cur_fname, len))
            return read_dentry_by_index(index, dentry); // If find the name, uses its index to call read_dentry_by_index
    }
    return -1;
}

/* read_data
 *
 * Helper funtion that reads data with specific length
 * from a file given its inode, offset.
 * Inputs: inode -- the index for the inode
 *         offset -- the starting position in a file to read from
 *         buf -- the buffer that takes the read data out
 *         length -- the number of bytes the data suppose to read
 * Outputs: Return the number read
 *          Return -1 if excesses the possible range of data
 * Side Effects: Save the value of file into buffer
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    if (!buf) return -1;
    inode_t file_inode = inode_start_addr[inode];   // the target inode block
    uint32_t file_len = file_inode.length;          // the length of file
    uint32_t buf_idx;                               // Loop counter
    uint32_t block_off, block_idx, data_idx;        // Several variable for index and offset
    if (offset > file_len) return -1;               // offset is larger than file length, return -1
    if ((offset + length) > file_len) length = file_len - offset; // If the file is shorter than the input length, adjust length

    for (buf_idx = 0; buf_idx < length; buf_idx++) {
        data_idx = (offset + buf_idx) % BLOCK_SIZE;     // find the offset in a data block
        block_off = (offset + buf_idx) / BLOCK_SIZE;    // find the offset of the data block in inode
        if (block_off >= d_block_num) return -1;        // check if the data block is in range
        block_idx = file_inode.data_block_num[block_off];   // find the actual index of the the data block
        buf[buf_idx] = d_block_start_addr[block_idx].data[data_idx];    // set the character to buffer
    }
    return buf_idx;     // return the bytes read
}

/* dir_read
 *
 * Read the name of a directory
 * Inputs: fd -- file descriptor number
 *         buf -- the buffer that takes the read data out
 *         nbytes -- the number of bytes the data suppose to read
 * Outputs: Return 0 for success
 *          Return -1 for index is invalid or buf is null
 * Side Effects: Save the name of file into buffer
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes){
    cli();
    dentry_t dentry;
    uint32_t ret = read_dentry_by_index(dir_idx, &dentry);      // call read_dentry_by_index to read the dentry
    if (ret == -1) return 0;
    dir_idx++;
    if (!buf) return -1;  // if buf is null, return -1
    strncpy((int8_t*)buf, dentry.filename, nbytes);
    strncpy((int8_t*)(buf + NAME_LEN), (int8_t*)"\0", 1);
    int32_t len = strlen((const int8_t*)buf);
    sti();
    return len;
}

/* file_read
 *
 * Read the content of file
 * Inputs: fd -- file descriptor number
 *         buf -- the buffer that takes the read data out
 *         nbytes -- the number of bytes the data suppose to read
 * Outputs: Return the number read
 *          Return -1 if excesses the possible range of data or buf is null
 * Side Effects: Save the name of file into buffer
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes){
    cli();
    fd_t* fda = get_fa();
    int32_t inode_idx = fda[fd].inode_idx;
    int32_t file_idx = fda[fd].file_pos;
    if (!buf) return -1;  // if buf is null, return -1
    
    uint32_t ret_data = read_data(inode_idx, file_idx, buf, nbytes);
    if (ret_data != -1) fda[fd].file_pos += ret_data;
    sti();
    return ret_data;
}

/* dir_close
 *
 * Close a directory
 * Inputs: fd -- file descriptor number
 * Outputs: Return 0
 * Side Effects: clear dir_idx to 0
 */
int32_t dir_close(int32_t fd) {
    dir_idx = 0;
    return 0;
}

/* dir_open
 *
 * Open directory
 * Not implemented yet
 */
int32_t dir_open(const uint8_t* dirname) {
    int32_t len = strlen((int8_t*) dirname);
    dir_idx = 0;
    while (strncmp(dentry_addr[dir_idx].filename, (int8_t*) dirname, len)) dir_idx++;
    return 0;
}


/* dir_write
 *
 * Bad syscall
 */
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}

/* file_close
 *
 * Close File
 * Do nothing specificly inside
 */
int32_t file_close(int32_t fd){
    return 0;
}

/* file_write
 *
 * Bad syscall
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}

/* file_open
 *
 * Open File
 * Do nothing specificly inside.
 */
int32_t file_open(const uint8_t* filename){
    return 0;
}

/* check_ececutable
 *
 * Check if a file is exectuable by checking is magic number at the beginning of the file
 * Input: fname -- File name
 * Output: Return 0 if it is executable
 *         Return -1 if it is not
 * Side Effect: None
 */
int32_t check_executable(const uint8_t* fname) {
    dentry_t dentry;
    int32_t ret = read_dentry_by_name(fname, &dentry);      
    if (ret == -1) return ret;
    int32_t inode_idx = dentry.inode_num;
    uint8_t magic_number1, magic_number2, magic_number3, magic_number4;        // there are four magic numbers signifying executable file
    ret = read_data(inode_idx, 0, &magic_number1, 1) & read_data(inode_idx, 1, &magic_number2, 1) &
          read_data(inode_idx, 2, &magic_number3, 1) & read_data(inode_idx, 3, &magic_number4, 1); // read four bytes of magic number from the beginning of the file
    if (ret == -1) return ret;
    // check magic number
    if (magic_number1 != 0x7F || magic_number2 != 0x45 || magic_number3 != 0x4C || magic_number4 != 0x46) 
        return -1;
    return 0;
}

/* exetract_ip
 *
 * exetract the entry point for a exectuable file from the file data
 * Input: fname -- File name
 * Output: Return ip if find valid entry point
 *         Return -1 if fail
 * Side Effect: None
 */
int32_t extract_ip(const uint8_t* fname) {
    dentry_t dentry;
    int32_t ret = read_dentry_by_name(fname, &dentry);
    if (ret == -1) return ret;
    int32_t inode_idx = dentry.inode_num;
    uint8_t ip_bytes1, ip_bytes2, ip_bytes3, ip_bytes4;
    ret = read_data(inode_idx, 24, &ip_bytes1, 1) & read_data(inode_idx, 25, &ip_bytes2, 1)&
          read_data(inode_idx, 26, &ip_bytes3, 1) & read_data(inode_idx, 27, &ip_bytes4, 1); // read four bytes of virtual address of first instruction
    if (ret == -1) return ret;
    // assemble the instruction address
    int32_t ip = (int32_t)ip_bytes1 + ((int32_t)ip_bytes2 << 8) + ((int32_t)ip_bytes3 << 16) + ((int32_t)ip_bytes4 << 24); 
    return ip;
}

/* load_executable
 *
 * Load the exeutable file to virtual memory address 0x8048000
 * Input: fname -- File name
 * Output: Return 0 if success
 *         Return -1 if fail
 * Side Effect: Copy the program image to 0x8048000
 */
int32_t load_executable(uint8_t *fname){
    dentry_t dentry;
    int32_t ret = read_dentry_by_name(fname, &dentry);
    if (ret == -1) return ret;
    int32_t inode_idx = dentry.inode_num;   
    uint32_t file_size = inode_start_addr[inode_idx].length; //obtain the file size
    ret = read_data(inode_idx, 0, (uint8_t*)PROG_V_ADDR, file_size); // copy the file to target loaction
    if (ret == -1) return ret;
    return 0;
}

/* init_fop_table
 *
 * initiate the file operation table
 * Input: None
 * Output: None
 * Side Effect: initiate the file operation table
 */
void init_fop_table(){
    dir_op_table.open = dir_open;
    dir_op_table.read = dir_read;
    dir_op_table.write = dir_write;
    dir_op_table.close = dir_close;
    file_op_table.open = file_open;
    file_op_table.read = file_read;
    file_op_table.write = file_write;
    file_op_table.close = file_close;
    rtc_op_table.open = rtc_open;
    rtc_op_table.read = rtc_read;
    rtc_op_table.write = rtc_write;
    rtc_op_table.close = rtc_close;
    stdout_op_table.write = terminal_write;
    stdin_op_table.read = terminal_read;
    // cannot be used
    stdin_op_table.open = NULL;
    stdin_op_table.write = NULL;
    stdin_op_table.close = NULL;
    stdout_op_table.open = NULL;
    stdout_op_table.read = NULL;
    stdout_op_table.close = NULL;
}


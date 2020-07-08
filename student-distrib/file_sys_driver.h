#include "types.h"
#include "lib.h"
#include "x86_desc.h"

// Magic numbers for the file system
#define BLOCK_SIZE 4096
#define BOOT_BLOCK_NUM 1
#define DENTRY_SIZE 64

#define FILE_TYPE 2
#define DIR_TYPE 1
#define RTC_TYPE 0

#define PROG_V_ADDR 0x8048000 

#define NAME_LEN 32

// Several globals for the file system structure
dentry_t* dentry_addr;
file_boot_block_t* b_block_addr;
inode_t* inode_start_addr;
data_block_t* d_block_start_addr;

// Initialize the filesystem
void init_fs(uint32_t fs_addr_start);

// helper functions
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

// system calls for files
int32_t file_close(int32_t fd);
int32_t file_open(const uint8_t* filename);
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

// system calls for directory
int32_t dir_close(int32_t fd);
int32_t dir_open(const uint8_t* dirname);
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

// helper funtion in syscall
int32_t check_executable(const uint8_t* fname);
int32_t extract_ip(const uint8_t* fname);
int32_t load_executable(uint8_t * fname);

void init_fop_table();

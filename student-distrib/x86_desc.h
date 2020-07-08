/* x86_desc.h - Defines for various x86 descriptors, descriptor tables,
 * and selectors
 * vim:ts=4 noexpandtab
 */

#ifndef _X86_DESC_H
#define _X86_DESC_H

#include "types.h"

/* Segment selector values */
#define KERNEL_CS   0x0010
#define KERNEL_DS   0x0018
#define USER_CS     0x0023
#define USER_DS     0x002B
#define KERNEL_TSS  0x0030
#define KERNEL_LDT  0x0038

/* Size of the task state segment (TSS) */
#define TSS_SIZE    104

/* Number of vectors in the interrupt descriptor table (IDT) */
#define NUM_VEC     256

/* Size of Page Directory and Page Table */
#define PG_SIZE     4096

/* Number of enteies in PTE and PDE. */
#define PG_NUM      1024

/* Length of Filename */
#define FILENAME_LEN 32

/* Maximun data block number for a inode */
#define DATA_B_NUM 1023

/* Maximum directory entries numbers */
#define DENTRY_NUM 63

/* Reserved number of Bytes in boot block */
#define RESERVED_BOOT 52

/* Reserved number of Bytes in dentry */
#define RESERVED_DENTRY 24

/* Maximum number of char can be saved in a data block */
#define DATA_BLOCK_LENGTH 4096

/* File Types */
#define USER_LVL_RTC 0
#define DIRECTORY 1
#define REGULR_FILE 2

/* Total count of the signals supported */
#define NUM_SIGNALS   5

#ifndef ASM

/* This structure is used to load descriptor base registers
 * like the GDTR and IDTR */
typedef struct x86_desc {
    uint16_t padding;
    uint16_t size;
    uint32_t addr;
} x86_desc_t;

/* This is a segment descriptor.  It goes in the GDT. */
typedef struct seg_desc {
    union {
        uint32_t val[2];
        struct {
            uint16_t seg_lim_15_00;
            uint16_t base_15_00;
            uint8_t  base_23_16;
            uint32_t type          : 4;
            uint32_t sys           : 1;
            uint32_t dpl           : 2;
            uint32_t present       : 1;
            uint32_t seg_lim_19_16 : 4;
            uint32_t avail         : 1;
            uint32_t reserved      : 1;
            uint32_t opsize        : 1;
            uint32_t granularity   : 1;
            uint8_t  base_31_24;
        } __attribute__ ((packed));
    };
} seg_desc_t;

/* TSS structure */
typedef struct __attribute__((packed)) tss_t {
    uint16_t prev_task_link;
    uint16_t prev_task_link_pad;

    uint32_t esp0;
    uint16_t ss0;
    uint16_t ss0_pad;

    uint32_t esp1;
    uint16_t ss1;
    uint16_t ss1_pad;

    uint32_t esp2;
    uint16_t ss2;
    uint16_t ss2_pad;

    uint32_t cr3;

    uint32_t eip;
    uint32_t eflags;

    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint16_t es;
    uint16_t es_pad;

    uint16_t cs;
    uint16_t cs_pad;

    uint16_t ss;
    uint16_t ss_pad;

    uint16_t ds;
    uint16_t ds_pad;

    uint16_t fs;
    uint16_t fs_pad;

    uint16_t gs;
    uint16_t gs_pad;

    uint16_t ldt_segment_selector;
    uint16_t ldt_pad;

    uint16_t debug_trap : 1;
    uint16_t io_pad     : 15;
    uint16_t io_base_addr;
} tss_t;

/* Some external descriptors declared in .S files */
extern x86_desc_t gdt_desc;

extern uint16_t ldt_desc;
extern uint32_t ldt_size;
extern seg_desc_t ldt_desc_ptr;
extern seg_desc_t gdt_ptr;
extern seg_desc_t gdt_desc_ptr;
extern uint32_t ldt;

extern uint32_t tss_size;
extern seg_desc_t tss_desc_ptr;
extern tss_t tss;

// Page Directory Entry structure
typedef struct __attribute__ ((packed)) pde {
    uint32_t present    : 1;
    uint32_t r_w        : 1;
    uint32_t u_s        : 1;
    uint32_t pwt        : 1;
    uint32_t pcd        : 1;
    uint32_t access     : 1;
    uint32_t dirty      : 1;
    uint32_t page_size  : 1;
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t addr       : 20;
} pde_t;

// Page Table Entry structure
typedef struct __attribute__ ((packed)) pte {
    uint32_t present    : 1;
    uint32_t r_w        : 1;
    uint32_t u_s        : 1;
    uint32_t pwt        : 1;
    uint32_t pcd        : 1;
    uint32_t access     : 1;
    uint32_t dirty      : 1;
    uint32_t pat        : 1;
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t addr       : 20;
} pte_t;

// Page table for video memory
pte_t video_mem_page_table[PG_NUM] __attribute__((aligned(PG_SIZE)));
// Page table for user level video memory
pte_t user_video_page_table[PG_NUM] __attribute__((aligned(PG_SIZE)));
// Page Directory
pde_t page_directory[PG_NUM] __attribute__((aligned(PG_SIZE)));

// The structure for directory entries, occupies 64B
typedef struct dentry{
    int8_t filename[FILENAME_LEN];
    int32_t filetype;
    int32_t inode_num;
    int8_t reserved[RESERVED_DENTRY];
} dentry_t;

// The structure for index node block
typedef struct inode{
    int32_t length;     // Length of data in Bytes, smaller than 2^22 bytes
    int32_t data_block_num[DATA_B_NUM];
} inode_t;

// The structure of boot block for file system
typedef struct file_boot_block{
    int32_t dir_cnt;
    int32_t inode_cnt;
    int32_t data_cnt;
    int8_t reserved[RESERVED_BOOT];
    dentry_t dir_entries[DENTRY_NUM];
} file_boot_block_t;

// The structure of data block for file system
typedef struct data_block{
    uint8_t data[DATA_BLOCK_LENGTH];
}data_block_t;

// The data structure for file operation table
typedef struct file_op_table{
    int32_t (*open)(const uint8_t*);
    int32_t (*read)(int32_t, void*, int32_t);
    int32_t (*write)(int32_t, const void*, int32_t);
    int32_t (*close)(int32_t);
} file_op_table_t;

// The structure for file descriptor
typedef struct file_descriptor{
    file_op_table_t* file_op_table_ptr;
    int32_t inode_idx; // 0 for directories and RTC
    int32_t file_pos; // saves virtual freq in rtc type
    int32_t flag;
} fd_t;

/* Data structures to handle signals */
typedef void (*sig_handler)(int signum);

// An array of pending signals (replacee the original linked list)
typedef struct sigpending{
    int32_t pending_signums[NUM_SIGNALS]; // array of pending signals
    int8_t head; // head of the pending signals
    int8_t tail; // tail of the pending signals
    uint32_t mask; // mask of all pending signals (0 for blocked)
    volatile int8_t lock; // pending the delivery if other signal is in process
} sigpending_t;

// Signal descriptor structure
typedef struct sighand{
    int8_t count; // count of processes using this structure (can be shared)
    sig_handler handler_table[NUM_SIGNALS]; // store the handlers for the
} sighand_t;

typedef struct process_contrl_block{
    fd_t fd_array[8]; // up to 8 file at most
    uint8_t current_id;    // current process idber
    uint8_t parent_id;  //parent process number for return
    uint32_t parent_pointer;
    uint32_t stack_p;   // stack pointer saved
    uint32_t stack_bp;  // stack base pointer
    uint32_t stack_switch_p;   // stack pointer saved
    uint32_t stack_switch_bp;  // stack base pointer
    uint8_t argument[128]; // arguments
    uint8_t terminal;
    
    // Store signal handling information
    sighand_t handler; // a descriptor for all the signals
    sigpending_t pending; // a queue of pending signals
    //sigset_t mask; // bitmask for blocked signals (0 for blocked)
} pcb_t;

// Define the file operation tables for different data type
file_op_table_t  dir_op_table;
file_op_table_t  file_op_table;
file_op_table_t  rtc_op_table;
file_op_table_t  stdin_op_table;
file_op_table_t  stdout_op_table;

/* Sets runtime-settable parameters in the GDT entry for the LDT */
#define SET_LDT_PARAMS(str, addr, lim)                          \
do {                                                            \
    str.base_31_24 = ((uint32_t)(addr) & 0xFF000000) >> 24;     \
    str.base_23_16 = ((uint32_t)(addr) & 0x00FF0000) >> 16;     \
    str.base_15_00 = (uint32_t)(addr) & 0x0000FFFF;             \
    str.seg_lim_19_16 = ((lim) & 0x000F0000) >> 16;             \
    str.seg_lim_15_00 = (lim) & 0x0000FFFF;                     \
} while (0)

/* Sets runtime parameters for the TSS */
#define SET_TSS_PARAMS(str, addr, lim)                          \
do {                                                            \
    str.base_31_24 = ((uint32_t)(addr) & 0xFF000000) >> 24;     \
    str.base_23_16 = ((uint32_t)(addr) & 0x00FF0000) >> 16;     \
    str.base_15_00 = (uint32_t)(addr) & 0x0000FFFF;             \
    str.seg_lim_19_16 = ((lim) & 0x000F0000) >> 16;             \
    str.seg_lim_15_00 = (lim) & 0x0000FFFF;                     \
} while (0)

/* An interrupt descriptor entry (goes into the IDT) */
typedef union idt_desc_t {
    uint32_t val[2];
    struct {
        uint16_t offset_15_00;
        uint16_t seg_selector;
        uint8_t  reserved4;
        uint32_t reserved3 : 1;
        uint32_t reserved2 : 1;
        uint32_t reserved1 : 1;
        uint32_t size      : 1;
        uint32_t reserved0 : 1;
        uint32_t dpl       : 2;
        uint32_t present   : 1;
        uint16_t offset_31_16;
    } __attribute__ ((packed));
} idt_desc_t;

/* The IDT itself (declared in x86_desc.S */
extern idt_desc_t idt[NUM_VEC];
/* The descriptor used to load the IDTR */
extern x86_desc_t idt_desc_ptr;

/* Sets runtime parameters for an IDT entry */
#define SET_IDT_ENTRY(str, handler)                              \
do {                                                             \
    str.offset_31_16 = ((uint32_t)(handler) & 0xFFFF0000) >> 16; \
    str.offset_15_00 = ((uint32_t)(handler) & 0xFFFF);           \
} while (0)

/* Load task register.  This macro takes a 16-bit index into the GDT,
 * which points to the TSS entry.  x86 then reads the GDT's TSS
 * descriptor and loads the base address specified in that descriptor
 * into the task register */
#define ltr(desc)                       \
do {                                    \
    asm volatile ("ltr %w0"             \
            :                           \
            : "r" (desc)                \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Load the interrupt descriptor table (IDT).  This macro takes a 32-bit
 * address which points to a 6-byte structure.  The 6-byte structure
 * (defined as "struct x86_desc" above) contains a 2-byte size field
 * specifying the size of the IDT, and a 4-byte address field specifying
 * the base address of the IDT. */
#define lidt(desc)                      \
do {                                    \
    asm volatile ("lidt (%0)"           \
            :                           \
            : "g" (desc)                \
            : "memory"                  \
    );                                  \
} while (0)

/* Load the local descriptor table (LDT) register.  This macro takes a
 * 16-bit index into the GDT, which points to the LDT entry.  x86 then
 * reads the GDT's LDT descriptor and loads the base address specified
 * in that descriptor into the LDT register */
#define lldt(desc)                      \
do {                                    \
    asm volatile ("lldt %%ax"           \
            :                           \
            : "a" (desc)                \
            : "memory"                  \
    );                                  \
} while (0)

#endif /* ASM */

#endif /* _x86_DESC_H */

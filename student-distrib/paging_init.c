/* paging_init.c - Functions used to initiate paging function
 */

#include "types.h"
#include "paging_init.h"
#include "x86_desc.h"

/* paging_init
 *
 * Set entries for page directory; initialize one page table
 * fill video memory and kernel in and enable paging
 * Inputs: None
 * Outputs: None
 * Side Effects: initialize paging tables
 */
void paging_init(void) {
    int i;          // Loop
    // Initialize the page directory to not present
    for (i = 0; i < PG_NUM; i++) {
        page_directory[i].present = 0;
        page_directory[i].r_w = 1;
        page_directory[i].u_s = 0;
        page_directory[i].pwt = 0;
        page_directory[i].pcd = 0;
        page_directory[i].access = 0;
        page_directory[i].dirty = 0;
        page_directory[i].page_size = 0;
        page_directory[i].global = 0;
        page_directory[i].available = 0;
        page_directory[i].addr = 0;
    }

    // Map the kernel to PDE
    page_directory[KERNEL_V_OFF].present = 1;
    page_directory[KERNEL_V_OFF].r_w = 1;
    page_directory[KERNEL_V_OFF].u_s = 0;
    page_directory[KERNEL_V_OFF].pwt = 0;
    page_directory[KERNEL_V_OFF].pcd = 0;
    page_directory[KERNEL_V_OFF].access = 0;
    page_directory[KERNEL_V_OFF].dirty = 0;
    page_directory[KERNEL_V_OFF].page_size = 1;
    page_directory[KERNEL_V_OFF].global = 1;
    page_directory[KERNEL_V_OFF].available = 0;
    page_directory[KERNEL_V_OFF].addr = KERNEL_OFF;

    // Initialize the page table to not present
    for (i = 0; i < PG_NUM; i++) {
        video_mem_page_table[i].present = 0;
        video_mem_page_table[i].r_w = 1;
        video_mem_page_table[i].u_s = 0;
        video_mem_page_table[i].pwt = 0;
        video_mem_page_table[i].pcd = 0;
        video_mem_page_table[i].access = 0;
        video_mem_page_table[i].dirty = 0;
        video_mem_page_table[i].pat = 0;
        video_mem_page_table[i].global = 0;
        video_mem_page_table[i].available = 0;
        video_mem_page_table[i].addr = 0;
    }

    // Map the PTE to video memory
    video_mem_page_table[VIDEO_START_OFF].present = 1;
    video_mem_page_table[VIDEO_START_OFF].r_w = 1;
    video_mem_page_table[VIDEO_START_OFF].u_s = 0;
    video_mem_page_table[VIDEO_START_OFF].pwt = 0;
    video_mem_page_table[VIDEO_START_OFF].pcd = 0;
    video_mem_page_table[VIDEO_START_OFF].access = 0;
    video_mem_page_table[VIDEO_START_OFF].dirty = 0;
    video_mem_page_table[VIDEO_START_OFF].pat = 0;
    video_mem_page_table[VIDEO_START_OFF].global = 0;
    video_mem_page_table[VIDEO_START_OFF].available = 0;
    video_mem_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF;

    // MAP the three buffer for video memory
    for(i=1; i<=TER_NUMBER; i++){
        video_mem_page_table[VIDEO_START_OFF+i].present = 1;
        video_mem_page_table[VIDEO_START_OFF+i].r_w = 1;
        video_mem_page_table[VIDEO_START_OFF+i].u_s = 0;
        video_mem_page_table[VIDEO_START_OFF+i].pwt = 0;
        video_mem_page_table[VIDEO_START_OFF+i].pcd = 0;
        video_mem_page_table[VIDEO_START_OFF+i].access = 0;
        video_mem_page_table[VIDEO_START_OFF+i].dirty = 0;
        video_mem_page_table[VIDEO_START_OFF+i].pat = 0;
        video_mem_page_table[VIDEO_START_OFF+i].global = 0;
        video_mem_page_table[VIDEO_START_OFF+i].available = 0;
        video_mem_page_table[VIDEO_START_OFF+i].addr = VIDEO_START_OFF+i;
    }
   
    
    // Put page table in the directory
    page_directory[VIDEO_V_OFF].present = 1;
    page_directory[VIDEO_V_OFF].r_w = 1;
    page_directory[VIDEO_V_OFF].u_s = 0;
    page_directory[VIDEO_V_OFF].pwt = 0;
    page_directory[VIDEO_V_OFF].pcd = 0;
    page_directory[VIDEO_V_OFF].access = 0;
    page_directory[VIDEO_V_OFF].dirty = 0;
    page_directory[VIDEO_V_OFF].page_size = 0;
    page_directory[VIDEO_V_OFF].global = 0;
    page_directory[VIDEO_V_OFF].available = 0;
    page_directory[VIDEO_V_OFF].addr = (unsigned int)video_mem_page_table >> SHIFT_OFF;

    // init user program paging
    init_user_program_pg();
    init_user_video_pg();

    // Set CR0, CR3 and CR4 in correct order
    enablePSE();                                // Enable page size extent
    loadPageDirectory((uint32_t*)page_directory);    // Load page directory base pointer to CR3
    enablePaging();                             // Enable paging
    enablePGE();                                // Enable paging global
}

/* init_user_program_pg
 *
 * Set entries for user program directory; initialize the 4MB page
 * Inputs: None
 * Outputs: None
 * Side Effects: initialize page directory
 */
void init_user_program_pg(void){
    page_directory[MB_128_V_OFF].present = 1;
    page_directory[MB_128_V_OFF].r_w = 1;
    page_directory[MB_128_V_OFF].u_s = 1;         // Set to user level
    page_directory[MB_128_V_OFF].pwt = 0;
    page_directory[MB_128_V_OFF].pcd = 0;
    page_directory[MB_128_V_OFF].access = 0;
    page_directory[MB_128_V_OFF].dirty = 0;
    page_directory[MB_128_V_OFF].page_size = 1;
    page_directory[MB_128_V_OFF].global = 1;
    page_directory[MB_128_V_OFF].available = 0;
    page_directory[MB_128_V_OFF].addr = 0;
    return;
}


/* init_user_video_pg
 *
 * Set entries for user program directory; 
 * initialize a page in the page directory.
 * Inputs: None
 * Outputs: None
 * Side Effects: initialize page director and page table
 */
void init_user_video_pg(void){
    int i;
    // Initialize the page table to not present
    for (i = 0; i < PG_NUM; i++) {
        user_video_page_table[i].present = 0;
        user_video_page_table[i].r_w = 1;
        user_video_page_table[i].u_s = 0;
        user_video_page_table[i].pwt = 0;
        user_video_page_table[i].pcd = 0;
        user_video_page_table[i].access = 0;
        user_video_page_table[i].dirty = 0;
        user_video_page_table[i].pat = 0;
        user_video_page_table[i].global = 0;
        user_video_page_table[i].available = 0;
        user_video_page_table[i].addr = 0;
    }
    // Map the PTE to video memory
    user_video_page_table[VIDEO_START_OFF].present = 0;
    user_video_page_table[VIDEO_START_OFF].r_w = 1;
    user_video_page_table[VIDEO_START_OFF].u_s = 1;   // Set to user level
    user_video_page_table[VIDEO_START_OFF].pwt = 0;
    user_video_page_table[VIDEO_START_OFF].pcd = 0;
    user_video_page_table[VIDEO_START_OFF].access = 0;
    user_video_page_table[VIDEO_START_OFF].dirty = 0;
    user_video_page_table[VIDEO_START_OFF].pat = 0;
    user_video_page_table[VIDEO_START_OFF].global = 0;
    user_video_page_table[VIDEO_START_OFF].available = 0;
    user_video_page_table[VIDEO_START_OFF].addr = VIDEO_START_OFF;
    
    // Put page table in the directory
    page_directory[USER_VIDEO_V_OFF].present = 0;
    page_directory[USER_VIDEO_V_OFF].r_w = 1;
    page_directory[USER_VIDEO_V_OFF].u_s = 1;    // Set to user level
    page_directory[USER_VIDEO_V_OFF].pwt = 0;
    page_directory[USER_VIDEO_V_OFF].pcd = 0;
    page_directory[USER_VIDEO_V_OFF].access = 0;
    page_directory[USER_VIDEO_V_OFF].dirty = 0;
    page_directory[USER_VIDEO_V_OFF].page_size = 0;
    page_directory[USER_VIDEO_V_OFF].global = 0;
    page_directory[USER_VIDEO_V_OFF].available = 0;
    page_directory[USER_VIDEO_V_OFF].addr = (unsigned int)user_video_page_table >> SHIFT_OFF;
    return;    
}

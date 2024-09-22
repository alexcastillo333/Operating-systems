#include "threads/thread.h"

/* Create a frame_table_entry 
    It should point to the page table entry that occupies it
    It should point to the frame it's responsible for.
    It should point to the page, if any, that is mapped to the frame.
    It should contain a boolean that determines if it is occupied or not. 
*/

// neal drive
struct frame_entry {
    struct thread * t; /* thread that is using this frame 
    (set to thread_current();) */
    void * upage;/* pointer to the user page that occupies this page 
    (set to pg_round_down( virtual address)) */
    void * kpage;// pointer to the physical page
};

void init_frame_table();

bool install_frame(struct thread * thread, void * upage, void * kpage);

void * get_kpage();
// end neal drive
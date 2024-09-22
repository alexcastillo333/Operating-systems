#include "vm/frame.h"
#include "threads/palloc.h"
#include "threads/thread.h"

#define MAX_KPAGES 367

static struct frame_entry frame_table[MAX_KPAGES] = {NULL, NULL, NULL};
static int clockhand = 0;
static struct frame_entry clock();


// Iterate 367 times, assigning a kpage to each frame_table
// alex drive
void init_frame_table() {
    for (int i = 0; i < MAX_KPAGES; i++) {
        void* kpage = palloc_get_page(PAL_USER);
        if (kpage == NULL) {
            printf("\nNo more pages.\n");
        } else {
            frame_table[i].kpage = kpage;
        }
    }
}
// end alex drive

// Return the frame_entry that has an accessed bit set to 0 
// neal drive
struct frame_entry clock() {
    ASSERT(clockhand < MAX_KPAGES && clockhand >= 0);
    uint32_t* pagedir = frame_table[clockhand].t->pagedir;
    void* upage = frame_table[clockhand].upage;
    struct frame_entry* result = NULL;
    while (!result) {
        if (!pagedir_is_accessed (pagedir, upage)) {
            result = &frame_table[clockhand];
        } else {
            pagedir_set_accessed (pagedir, upage, false);
        }
        clockhand = (clockhand + 1) % MAX_KPAGES;
        pagedir = frame_table[clockhand].t->pagedir;
        upage = frame_table[clockhand].upage;
    }
    ASSERT(result != NULL);
    return *result;
}
// end neal drive


 // get a kpage from the user pool  
 // iterate thrugh the frame table, returning the first unused frame, or 
 // run the eviction algorithm to evict a frame first.
 // alex drive
void * get_kpage() {
    // Iterate through frame_table
    for (int i = 0; i < MAX_KPAGES; i++) {
        // How do we know when a kpage is free?
        if (frame_table[i].upage == NULL && frame_table[i].t == NULL) {
            // This is an unmapped kpage
            return frame_table[i].kpage;
        }
    }

    // If we don't find anything, then we call the eviction process
    struct frame_entry frame_to_evict = clock();
    uint32_t* pagedir = frame_to_evict.t->pagedir;
    void* upage = frame_to_evict.upage;
    if (pagedir_is_dirty((pagedir, upage))) {
        // We will write to swap before we give back the kpage.
        printf("we need to write to swap\n");
        //swap_write(&frame_to_evict);
    } 
    // We need to make sure a process does not run when we are evicting its page
    pagedir_clear_page(pagedir, upage);
    // After removing the mapping, we need to make a new mapping
    return frame_to_evict.kpage;   
}
// end alex drive

// neal drive
bool install_frame(struct thread * thread, void * upage, void * kpage) {
    for (int i = 0; i < 367; i++) {
        if (frame_table[i].kpage == kpage) {
            frame_table[i].t = thread;
            frame_table[i].upage = upage;
            return true;
        }
    }
    return false;
}
// end neal drive




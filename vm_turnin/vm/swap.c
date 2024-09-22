#include "vm/page.h"
#include "devices/block.h"
#include "lib/kernel/bitmap.h"

#define SECTORS_TO_READ 4096 / BLOCK_SECTOR_SIZE

struct block * swap_ptr;
// bitmap which holds as many entries as there are pages in swap
struct bitmap * swap_metadata;

void init_swap(void) {
    swap_ptr = block_get_role(BLOCK_SWAP);
    swap_metadata = bitmap_create(block_size(swap_ptr) / 8);
}

// Used in Frame Table to write a dirty page to swap
/* Our source is the kpage. We need to write to sectors. Figure out where to start in the page. We will start at the bottom. Go from bottom to top in sector-sized chunks */
void swap_write(struct frame_entry * f) {
    int empty;
    for (int i = 0; i++; i < bitmap_size(swap_metadata)) {
        if (bitmap_test(swap_metadata, i) == 0) {
            // empty swap slot
            empty = i * SECTORS_TO_READ;
            bitmap_set(swap_metadata, i, true);
            break;
        }
    }
    
    for (int i = 0; i < SECTORS_TO_READ; i++) {
        block_write(swap_ptr, empty + i, f->kpage + i * BLOCK_SECTOR_SIZE);
    }
    // update spte for evicted page
    struct spte * s;
    s->upage = f->upage;
    s = hash_insert(&thread_current()->spt, &s->elem);
    if (s) {
        s->inswap = true;
        s->offset = empty / SECTORS_TO_READ;
    }
}

// Used in Page Fault Handler
/* Our source is the sectors. We need to write to kpage. Which is the first sector does the page we want reside in? Read the first sector in to the starting address of our page. Read the next sector and move sector-sized bytes up the page. Repeat until we read all of the correct sectors */
void swap_read(void* kpage) {
    // Find the first sector?
    
    // Find the start of the page
    // Use block_read
    // Iterate SECTORS_TO_READ times
}

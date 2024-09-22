#include <hash.h>
#include "filesys/file.h"
#include "devices/block.h"
#include "vm/frame.h"
#include "threads/thread.h"

// supplemental page table entry
/* We need to know where the page exists. In Swap or Filesys
   We need to know what to remove using the SPT */
struct spte {
    struct hash_elem elem; 
    void * upage; // user page
    bool inswap; // is page in swap or filesys
    struct file * file; // if the page is in a file in the filesys
    int offset; /* if page is in a file, this is offset of the page into that 
    file */
};


void init_spt(struct thread * t);


bool map_spte(struct hash * spt, void * upage);

void free_data();

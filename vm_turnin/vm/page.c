#include "vm/page.h"
#include <hash.h>
#include "threads/vaddr.h"

// alex drive
static bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux UNUSED);
static unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED);


void init_spt(struct thread * t) {
    hash_init(&t->spt, page_hash, page_less, NULL);
}

struct spte * init_spte() {
    struct spte * s = malloc(sizeof(struct spte));

}

static bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux UNUSED) {
    struct spte *a = hash_entry (a_, struct spte, elem);
    struct spte *b = hash_entry (b_, struct spte, elem);
    return a->upage < b->upage;

}

static unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED) {
    struct spte *p = hash_entry (p_, struct spte, elem);
    return hash_bytes (&p->upage, sizeof p->upage);
}

// end alex drive

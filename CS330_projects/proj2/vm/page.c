#include "vm/page.h"
#include <list.h>
#include "threads/thread.h"

/*
 * Initialize supplementary page table
 */
void 
page_init (void)
{
  list_init(&thread_current()->spt);
}

/*
 * Make new supplementary page table entry for addr 
 */

/*
 * SPTE : usr_vaddr / access_time / dirty,accessed / memory_type,on_memory / permission
 *        / swap_sector / file,start,end
 */
struct sup_page_table_entry *
allocate_page (void *addr)
{
  spte* new_entry = (spte *) malloc(sizeof(spte));

  // Round it down
  new_entry->user_vaddr = (uint32_t *) ((((int) addr) >> 12) << 12);
  new_entry->access_time = timer_ticks();

  new_entry->on_memory = true;

  return new_entry;
}

/* 
 * find_page
 * lookup the page from SPTE of current thread
 */
spte* find_page(void* addr) {
  uint32_t* addr_rnd = (uint32_t *) ((((int) addr) >> 12) << 12);

  struct list_elem* e;
  for(e = list_begin(&thread_current()->spt); e != list_end(&thread_current()->spt); e = list_next(e)) {
    spte* spte_ = list_entry(e, spte, elem);
    if(addr_rnd == spte_->user_vaddr) {
      return spte_;
    }
  }
  return NULL;
}



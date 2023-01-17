#include "vm/page.h"
#include "vm/frame.h"
#include <list.h>

#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "threads/malloc.h"

#define PGADDR(x) (((uint32_t) x) & (~0xfff))

struct lock ft_lock;
struct list frame_table;
//bool install_page(void* upage, void* kpage, bool writable);

/*
 * Initialize frame table
 */
void 
frame_init (void)
{
  lock_init(&ft_lock);
  list_init(&frame_table);
}


/* 
 * Make a new frame table entry for addr.
 */
bool
allocate_frame (void *addr)
{
  struct thread* t = thread_current ();

  void* new_page = palloc_get_page(PAL_USER);

//  pagedir_set_page(t->pagedir, PGADDR(addr), new_page, true);

  if(new_page) {
    struct frame_table_entry* fte = (struct frame_table_entry *) malloc(sizeof(struct frame_table_entry));
    fte->frame = new_page;

    spte* new_spte = allocate_page(addr);
    fte->spte = new_spte;
    fte->owner = t;

//    printf("install page : USER_ADDR = 0x%x -> KERNEL_ADDR = 0x%x\n", PGADDR(addr), new_page); 
    if(!install_page(PGADDR(addr), new_page, true))
      printf("install page failed!\n");

    lock_acquire(&ft_lock);
    list_push_front(&frame_table, &fte->elem);
    lock_release(&ft_lock);

    return true;

  } else {
    return false;
    // PANIC("NO FRAME!");
  }
}

struct frame_table_entry* find_frame (void *addr) {
  struct list_elem* e;
  
  uint32_t* addr_eff = (uint32_t *) ((((int) addr) >> 12) << 12);

  for(e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e)) {
    struct frame_table_entry* fte_ptr = list_entry(e, struct frame_table_entry, elem);
    if(addr_eff == (fte_ptr->frame)) {
      printf("compare : list<0x%x> / search<0x%x> \n", fte_ptr->frame, addr_eff);
      return fte_ptr;
    }
  }
  return NULL;
}

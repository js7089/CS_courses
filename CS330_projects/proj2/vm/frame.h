#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdbool.h>
#include <stdint.h>
#include <list.h>

#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/thread.h"

struct frame_table_entry
{
	uint32_t* frame;
	struct thread* owner;
	struct sup_page_table_entry* spte;

  struct list_elem elem;
};

void frame_init (void);
bool allocate_frame (void *addr);
struct frame_table_entry* find_frame(void *addr);

#endif /* vm/frame.h */

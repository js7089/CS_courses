#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <list.h>
#include <stdint.h>
#include <stdbool.h>

enum memory_type {
  MMAP, FILE, SWAP, STACK
};

enum permission {
  R, RW
};

struct sup_page_table_entry 
{
	uint32_t* user_vaddr;
	uint64_t access_time;

	bool dirty;
	bool accessed;

  struct list_elem elem;

  enum memory_type type;  // mmap, file, swap, stack
  bool on_memory;         // main memory(true), swap(false)

  enum permission rw;

  size_t swap_sector;     // in swap

/* Managing open file
           [page]
  |00|#### data ####|00000|
0KB  ^              ^     4KB
   start           end
 */

  struct file* file;      // open file
  size_t read_bytes;      // file starts
  size_t zero_bytes;      // file ends
  size_t ofs;

};

typedef struct sup_page_table_entry spte;

void page_init (void);
struct sup_page_table_entry *allocate_page (void *addr);

#endif /* vm/page.h */

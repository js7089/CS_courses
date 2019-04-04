#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#define DEBUG 0

static void syscall_handler (struct intr_frame *);
int valid(const void* vaddr);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int* p = f->esp;
  int sig = *p;

  if(DEBUG)  printf("signal #%d\n",sig);

  switch (sig) {
    // void halt()
    case SYS_HALT:
    {
      power_off();
      break;
    }

    // void exit(int status)
    case SYS_EXIT:
    {


      struct thread* this = thread_current();
      if(DEBUG){
        printf("exit(%d) called", *(p+1));
        printf("by tid=%d\n",thread_current()->tid);
      }
      printf("%s: exit(%d)\n",this->name, this->exit_status);

      list_remove(&thread_current()->elem2);

      thread_current()->parent->exit_status = *(p+1);

      sema_up(&thread_current()->parent->child_sema);
      thread_exit();
      break;
    }

    // pid_t exec(const char* cmd_line)
    case SYS_EXEC:

    // int wait(pid_t pid)
    case SYS_WAIT:
    {
      if(*(p+1) > 0xc0000000){
        thread_exit();
        printf("bad!\n");
        return -1;
      }
        
      process_wait(*(p+1));

//      printf("wait for(%d)\n",*(p+1));
/*
      struct thread* tp = thread_current();
      struct list_elem* e;
      for(e=list_begin(&tp->children); e!=list_end(&tp->children); e=list_next(e)){
        struct thread* child = list_entry(e, struct thread, elem2);
        printf("tid=%d\n",child->tid);
      }
*/
      break;
    }

    // bool create(const char* file, unsigned initial_size)
    case SYS_CREATE:
      break;

    // bool remove(const char* file)
    case SYS_REMOVE:
      break;

    // int open(const char* file)
    case SYS_OPEN:
      break;

    // int filesize(int fd)
    case SYS_FILESIZE:
      break;

    // int read(int fd, void* buffer, unsigned size)
    case SYS_READ:
    {
      int fd = *(p+1);

      break; 
    }

    // int write(int fd, const void* buffer, unsigned size)
    case SYS_WRITE:
    {
      int fd = *(p+1);
      char* usrbuf = *(p+2);
      size_t slen = *(p+3);
      
      if(DEBUG) printf("write(fd=%d, usrbuf=0x%x, slen=%d)\n", fd, usrbuf, slen); 

      if (fd == 1) {  // write on console
        putbuf((char*) usrbuf, slen);
        f->eax = slen;
      } else {
        printf("[SYS_WRITE] system call!\n");



      }
      break;
    }

    // void seek(int fd, unsigned position)
    case SYS_SEEK:
      break;  

    // unsigned tell(int fd)
    case SYS_TELL:
      break;  

    // void close(int fd)
    case SYS_CLOSE:
      break;  

    default:
      printf("system call!\n");
      break;
  }

//  printf ("system call!\n");
//  thread_exit ();
}


int valid(const void* vaddr){
  // not user space (PHYS_BASE)
  if(!is_user_vaddr(vaddr))
    return 0;

  // unmapped
  if(!pagedir_get_page(thread_current()->pagedir, vaddr))
    return 0;

  return 1;
}

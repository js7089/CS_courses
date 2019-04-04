#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#define DEBUG 0

static void syscall_handler (struct intr_frame *);

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
    case SYS_HALT:
    {
      power_off();
      break;
    }

    case SYS_EXIT:
    {
      struct thread* this = thread_current();
      if(DEBUG){
        printf("exit(%d) called", *(p+1));
        printf("by tid=%d\n",thread_current()->tid);
      }
      thread_current()->exit_status = *(p+1); 

      printf("%s: exit(%d)\n",this->name, this->exit_status);
      sema_up(&thread_current()->parent->child_sema);
      thread_exit();
      break;
    }

    case SYS_EXEC:

    case SYS_WAIT:
    {
      printf("SYSCALL WAIT\n");
      process_wait(*(p+1));
      struct thread* tp = thread_current();
      struct list_elem* e;
      for(e=list_begin(&tp->children); e!=list_end(&tp->children); e=list_next(e)){
        struct thread* child = list_entry(e, struct thread, elem2);
        printf("tid=%d\n",child->tid);
      }
      break;
    }
    case SYS_CREATE:
      break;

    case SYS_REMOVE:
      break;

    case SYS_OPEN:
      break;

    case SYS_FILESIZE:
      break;

    case SYS_READ:
    {
      int fd = *(p+1);
      

      break; 
    }
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
    case SYS_SEEK:
      break;  
    case SYS_TELL:
      break;  

    case SYS_CLOSE:
      break;  

    default:
      printf("system call!\n");
      break;
  }

//  printf ("system call!\n");
//  thread_exit ();
}

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

  if(DEBUG+1)  printf("signal #%d\n",sig);

  switch (sig) {
    case SYS_HALT:
    {
      power_off();
      break;
    }

    case SYS_EXIT:
    {
      printf("exit(%d) called", sig+1);
      thread_exit();
      break;
    }

    case SYS_EXEC:

    case SYS_WAIT:
    {
      printf("wait(pid=%d)\n",p+4);
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

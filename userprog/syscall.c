#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

#define DEBUG 0

static void syscall_handler (struct intr_frame *);
int valid(const void* vaddr);
void abort_userprog(void);

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
      printf("%s: exit(%d)\n",this->name, *(p+1));

      list_remove(&thread_current()->elem2);

      thread_current()->parent->exit_status = *(p+1);
      sema_up(&thread_current()->parent->child_sema);
      thread_exit();
      break;
    }

    // pid_t exec(const char* cmd_line)
    case SYS_EXEC:
    { 
      if(!valid(*(p+1))){
        f->eax = -1;
        return;
      }
      char* file_name = (char*) *(p+1);

      char* fn_cp = malloc(strlen(file_name)+1);
      strlcpy(fn_cp, file_name, strlen(file_name)+1);

      char* next;
      fn_cp = strtok_r(fn_cp, " ", &next);

      struct file* fp = filesys_open(fn_cp);

      if(!fp){
        f->eax = -1;
      }else{
        f->eax = process_execute(file_name);
        file_close(fp);
      }
      break;
    }


    // int wait(pid_t pid)
    case SYS_WAIT:
    {
      f->eax = process_wait(*(p+1));

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
      if(!valid(*(p+1))){
        abort_userprog();
        f->eax = -1;
        return;
      }
      
      

      break;

    // int filesize(int fd)
    case SYS_FILESIZE:
      break;

    // int read(int fd, void* buffer, unsigned size)
    case SYS_READ:
    {
      int fd = *(p+1);
      char* buf = *(p+2);
      unsigned slen = *(p+3);

      break; 
    }

    // int write(int fd, const void* buffer, unsigned size)
    case SYS_WRITE:
    {
      if(!valid(*(p+2))){
        abort_userprog();
        break;
      }
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

void abort_userprog(){
  list_remove(&thread_current()->elem2);
  thread_current()->parent->exit_status = -1;
  sema_up(&thread_current()->parent->child_sema);
  thread_exit();
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

#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/init.h"

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
  uint32_t* p = f->esp;
  int sig = *(int *)p;

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
      if(!valid(p+1)){
        abort_userprog();
        f->eax = -1;
        return;
      }

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
      if(!valid(*(p+1)) || !valid(p+1)){
        f->eax = -1;
        return;
      }
      char* file_name = (char*) *(p+1);

      char* fn_cp = malloc(strlen(file_name)+1);
      strlcpy(fn_cp, file_name, strlen(file_name)+1);

      char* next;
      fn_cp = strtok_r(fn_cp, " ", &next);

      lock_acquire(&filesys_lock);

      struct file* fp = filesys_open(fn_cp);
      if(!fp){
        f->eax = -1;
      }else{
        f->eax = process_execute(file_name);
        file_close(fp);
      }

      lock_release(&filesys_lock);
      free(fn_cp);
      break;
    }


    // int wait(pid_t pid)
    case SYS_WAIT:
    {
      if(!valid(p+1)){
        abort_userprog();
        f->eax = -1;
        return;
      }
      f->eax = process_wait(*(p+1));

      break;
    }

    // bool create(const char* file, unsigned initial_size)
    case SYS_CREATE:
    {
      if(!valid(*(p+1)) || !valid(p+2)){
        abort_userprog(); 
        f->eax = 0;
        return;
      }
      char* file = (char *) *(p+1);
      unsigned initial_size = *(p+2);

      if(!strlen(file)){
        f->eax = 0;
        return;
      }
      if(!file){
        abort_userprog();
        f->eax = -1;
        return;
      }

      lock_acquire(&filesys_lock);
      f->eax = filesys_create(file, initial_size);
      lock_release(&filesys_lock);

      break;
    }
    // bool remove(const char* file)
    case SYS_REMOVE:
    {
      if(!valid(p+1) || !valid(p+1)){
        abort_userprog();
        f->eax = 0;
        return;
      }
      
      
      break;
    }
    // int open(const char* file)
    case SYS_OPEN:
    {
      if(!valid((char *) *(p+1)) || !valid(p+1)){
        abort_userprog();
        f->eax = -1;
        return;
      }
      lock_acquire(&filesys_lock);

      char* filename = *(p+1);
      if(!filename){
        f->eax = -1;
        lock_release(&filesys_lock);
        return;
      }
      struct file* openfile = filesys_open(filename);
      if(!openfile){  // open() fails.
        f->eax = -1;
        lock_release(&filesys_lock);
        return;
      }
      
      // create a new descriptor node and put it in current thread's list
      struct descriptor* newnode = (struct descriptor *) malloc(sizeof(struct descriptor));
      memset(newnode, 0, sizeof(struct descriptor));
      newnode->fp = openfile;
      thread_current()->open_cnt++;
      newnode->fd = thread_current()->open_cnt;
      list_push_back(&thread_current()->files, &newnode->elem);
      lock_release(&filesys_lock);
      f->eax = newnode->fd;

      break;
    }
    // int filesize(int fd)
    case SYS_FILESIZE:
    {
      if(!valid(p+1)){
        abort_userprog();
        f->eax = -1;
        return;
      }

      int fd = *(p+1);

      struct list_elem* e;
      lock_acquire(&filesys_lock);
      for( e = list_begin(&thread_current()->files); e != list_end(&thread_current()->files); e = list_next(e) ){
        struct descriptor* node = list_entry(e, struct descriptor, elem);
        if(node->fd == fd){
          f->eax = file_length(node->fp);
          break;
        }
      }
      if( list_entry(e, struct descriptor, elem)->fd!=fd && e==list_end(&thread_current()->files)){
        f->eax = -1;
      }
      lock_release(&filesys_lock);
      break;
    }
    // int read(int fd, void* buffer, unsigned size)
    case SYS_READ:
    {
      if(!valid(*(p+2)) || !valid(p+3)){
        abort_userprog();
        f->eax = 0;
        return;
      }
      int fd = *(p+1);
      char* buf = *(p+2);
      unsigned slen = *(p+3);

      if(!fd) {   // read from stdin
        size_t written = 0;
        while(written < slen){
          buf[written++] = input_getc();
        }
        f->eax = written;
      } else {
        lock_acquire(&filesys_lock);
        struct list_elem* e;
        for( e = list_begin(&thread_current()->files); e != list_end(&thread_current()->files); e = list_next(e) ){ 
          struct descriptor* node = list_entry( e, struct descriptor, elem);
          if( node->fd == fd ) {  // valid FD
            size_t len = file_read(node->fp, buf, slen);
            f->eax = (int) len;
            break;
          }
        } 
        
        if( list_entry(e, struct descriptor, elem)->fd!=fd && e == list_end(&thread_current()->files)){
            f->eax = 0;
        }
        lock_release(&filesys_lock);

      }
      break; 
    }

    // int write(int fd, const void* buffer, unsigned size)
    case SYS_WRITE:
    {
      if(!valid((char *)*(p+2)) || !valid(p+3)){
        abort_userprog();
        f->eax = -1;
        break;
      }

      int fd = *(int *)(p+1);
      char* usrbuf = *(p+2);
      size_t slen = *(size_t *)(p+3);

      if(DEBUG) printf("write(fd=%d, usrbuf=0x%x, slen=%d)\n", fd, usrbuf, slen); 

      if (fd == 1) {  // write on console
        putbuf((char*) usrbuf, slen);
        f->eax = slen;
      } else {
        struct list_elem* e;
        lock_acquire(&filesys_lock);
        for( e = list_begin(&thread_current()->files) ; e != list_end(&thread_current()->files); e = list_next(e) ){
          struct descriptor* node = list_entry(e, struct descriptor, elem);
          if(node->fd == fd){ // case valid FD
            f->eax = file_write(node->fp, usrbuf, slen);
            break;
          }
        }
        if( list_entry(e, struct descriptor, elem)->fd != fd && e == list_end(&thread_current()->files) ){  // invalid FD
          f->eax = 0;
        }
        lock_release(&filesys_lock);
 //       printf("[SYS_WRITE] system call!\n");



      }
      break;
    }

    // void seek(int fd, unsigned position)
    case SYS_SEEK:
    {
      if(!valid(p+2)){
        abort_userprog();
        f->eax = -1;
        break;
      }
      int fd = *(p+1);
      unsigned position = *(unsigned *)(p+2);

      lock_acquire(&filesys_lock);
      struct list_elem* e;
      for( e = list_begin(&thread_current()->files); e != list_end(&thread_current()->files); e = list_next(e) ){
        struct descriptor* node = list_entry(e, struct descriptor, elem);
        if(node->fd == fd){
          file_seek(node->fp, position);
          break;
        }
      }
      lock_release(&filesys_lock);      

      break;  
    }
    // unsigned tell(int fd)
    case SYS_TELL:
    {
      if(!valid(p+1)){
        abort_userprog();
        f->eax = -1;
        break;
      } 

      int fd = *(p+1);

      lock_acquire(&filesys_lock);
      struct list_elem* e;
      for( e = list_begin(&thread_current()->files); e != list_end(&thread_current()->files); e = list_next(e) ){
        struct descriptor* node = list_entry(e, struct descriptor, elem);
        if(node->fd == fd){
          f->eax = file_tell(node->fp);
          break;
        }
      }
      lock_release(&filesys_lock);

      break;  
    }

    // void close(int fd)
    case SYS_CLOSE:
      // check for invalid address
      if(!valid(p+1)){
        abort_userprog();
        break;
      }
      // find from current open FD list
      // if there is, close the file and return
      // otherwise, do nothing.
      int fd = *(p+1);

      lock_acquire(&filesys_lock);
      struct list_elem* e;
      for( e = list_begin(&thread_current()->files); e != list_end(&thread_current()->files); e = list_next(e) ){
        struct descriptor* node = list_entry(e, struct descriptor, elem);
        if(fd == node->fd){
          file_close(node->fp);
          list_remove(e);
          break;
        }
      }
      lock_release(&filesys_lock);
      break;  

    default:
      printf("system call!\n");
      break;
  }

//  printf ("system call!\n");
//  thread_exit ();
}

void abort_userprog(){
  struct thread* par = thread_current()->parent;
  list_remove(&thread_current()->elem2);
  par->exit_status = -1;
  sema_up(&par->child_sema);
  printf("%s: exit(%d)\n", thread_current()->name, par->exit_status);
  thread_exit();
}

int valid(const void* vaddr){
  // not user space (PHYS_BASE)
  if(!is_user_vaddr(vaddr))
    return 0;

  void* paddr = pagedir_get_page(thread_current()->pagedir, vaddr);

  // unmapped 
  if(!pagedir_get_page(thread_current()->pagedir, vaddr) )
    return 0;
 

  return 1;
}

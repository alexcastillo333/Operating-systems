#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include <stdlib.h>
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "lib/kernel/stdio.h"
#include "process.h"
#include "threads/palloc.h"


struct lock file_lock;
static void syscall_handler (struct intr_frame *);
static void check_buffer (char * buffer);




// checks that all bytes in a buffer are in user virtual address space
void check_buffer (char * buffer) {
  int size = strlen(buffer);
  char * temp = buffer;
  //printf("%s\n", buffer);
  // check that each byte on a multiple of page size is in user virtual add space
  for (int i = 0; i < size; i += 4096) {
    //if (!is_user_vaddr((void *)temp)) {
    //  our_exit(-1);
    //}
    check_addr(temp);
    temp += 4096;
  }
  // check the end byte;
  temp = buffer + size + 1;
  //if (!is_user_vaddr((void *)temp)) {
  //    our_exit(-1);
  //}
  check_addr(temp);
}


/* Our Implementation: */


void syscall_init (void) 
{ 
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}



/* Exit helper method */
void our_exit(int status) { 
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_current()->ret_status = status;
  sema_up(&thread_current()->parent_wait);
  sema_down(&thread_current()->child_wait);
  //if process is a parent, release any children that may still be waiting
  struct list_elem *e;
  for (e = list_begin (&thread_current()->child_list); e != list_end (&thread_current()->child_list);
      e = list_next (e)) {
    struct thread *child = list_entry (e, struct thread, child_elem);
    sema_up(&child->child_wait);
  }
  //then remove children
  for (e = list_begin (&thread_current()->child_list); e != list_end (&thread_current()->child_list); e = list_remove (e));
  thread_exit();
}

/* utility function for testing addr ptr */
void check_addr(char *addr){
  // Is addr null, is the addr in kernel vaddr space, is addr unmapped?
  if (addr == NULL || !is_user_vaddr((void*)(addr)) || !pagedir_get_page(thread_current()->pagedir, (void*)addr)){
    if(lock_held_by_current_thread(&file_lock)) {
      lock_release(&file_lock);
    }
    our_exit(-1);
  }
  //continue
}

/* Wait helper meethod */
int wait(tid_t pid){
  int wait_ret = -1;
  //check for valid tid
  wait_ret = process_wait(pid);
  return wait_ret;
}

/* Create helper method */
bool create(const char *file, unsigned size){
  bool success = false;

  // valid arguments
  if(size && file){ 
    lock_acquire(&file_lock);
    success = filesys_create(file, size);
    lock_release(&file_lock);
  } 
  return success;
}

/* Remove helper method */
bool remove(const char *file){
  bool success = false;

  // valid argument
  if (file){
    lock_acquire(&file_lock);
    success = filesys_remove(file);
    lock_release(&file_lock);
  }
  return success;
}

/* Open helper method */
int open(const char *file){
  int fd_ret = -1;
  return fd_ret;
}

/* Filesize helper method */
int filesize(int fd){
  int size = -1;
  // valid fd argument
  if (&fd && fd > 1 && fd < 130){
    struct file *open = thread_current()->file_list[fd - 2];
    if (open){
      lock_acquire(&file_lock);
      struct inode *cur_inode = file_get_inode(open);
      if (cur_inode){
        size = inode_length(cur_inode);
      } else {
        our_exit(-1);
      }
      lock_release(&file_lock);
    } else {
      our_exit(-1);
    }
  } else {
    our_exit(-1);
  }
  return size;
}

/* Read helper method */
int read(int fd, char *buffer, unsigned size){
  int num_bytes = -1;
  //if (!check_buffer (buffer)) {
  //  our_exit(-1);
  //}
  check_buffer(buffer);
  // Valid arguments
  if(&fd && (fd == 0 || fd > 1) && fd < 130 && &size){
    if (fd == 0){ 
      //input_getc()
    } else {
      struct file* to_read = thread_current()->file_list[fd - 2];
      if(to_read) {
        lock_acquire(&file_lock);
        num_bytes = file_read(to_read, buffer, size);
        lock_release(&file_lock);
      } else {
        our_exit(-1);
      }
    }
  } else {
    our_exit(-1);
  }
  return num_bytes;
}

/* Seek helper method */
void seek(int fd, unsigned position){
  // Valid argument
  if (&fd && fd > 1 && fd < 130 && &position){
    struct file* to_seek = thread_current()->file_list[fd - 2];
    if(to_seek) {
      lock_acquire(&file_lock);
      file_seek(to_seek, position);
      lock_release(&file_lock);
    } else {
      our_exit(-1);
    }
  } else {
    our_exit(-1);
  }
}

/* Tell helper method */
unsigned tell(int fd){
  int tell_ret = -1;
  // Valid argument
  if(&fd && fd > 1 && fd < 130){
    struct file* to_tell = thread_current()->file_list[fd - 2];
    if(to_tell) {
      lock_acquire(&file_lock);
      tell_ret = file_tell(to_tell);
      lock_release(&file_lock);
    } else {
      our_exit(-1);
    }
  } else {
    our_exit(-1);
  }
  return tell_ret;
}

void close(int fd){
  if(&fd && fd > 1 && fd < 130) {
    struct file* to_close = thread_current()->file_list[fd - 2];
    if(to_close) {
      lock_acquire(&file_lock);
      file_close(to_close);
      lock_release(&file_lock);
      thread_current()->file_list[fd - 2] = NULL;
    } else {
      our_exit(-1);
    }
  } else {
    our_exit(-1);
  }
}

static void
// alex drive, updating thread_current->esp before dereferenes
syscall_handler (struct intr_frame *f) 
{
  // Our implementation:
  char* file;
  int fd;
  unsigned size;
  char* buffer;
  bool success;
  char *myEsp = f->esp;
  check_addr(myEsp);
  tid_t tid;
  /* System call infrastructure */
  /* Get variables and let helper methods finish */
  switch (*myEsp){
    case SYS_HALT: 
      shutdown_power_off();
      break;
    case SYS_EXIT:  /* Terminate this process. */
      //printf("\nexit sisteym call\n");
      myEsp += 4;
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      int status = *(int*)myEsp;
      if(!&status) {
        our_exit(-1);
      } else {
        printf("%s: exit(%d)\n", thread_current()->name, status);
        lock_acquire(&file_lock);
        file_close(thread_current()->elf);
        lock_release(&file_lock);
        thread_current()->ret_status = status;
        sema_up(&thread_current()->parent_wait);
        sema_down(&thread_current()->child_wait);
        //if process is a parent, release any children that may still be waiting
        struct list_elem *e;
        for (e = list_begin (&thread_current()->child_list); e != list_end (&thread_current()->child_list);
            e = list_next (e)) {
          struct thread *child = list_entry (e, struct thread, child_elem);
          sema_up(&child->child_wait);
        }
        //then remove children
        for (e = list_begin (&thread_current()->child_list); e != list_end (&thread_current()->child_list); e = list_remove (e));
        //thread_exit();
        our_exit(0);
      }
      break;  
    case SYS_EXEC:
      //get params
      myEsp += sizeof(int);
      check_addr(myEsp);
      check_addr(myEsp + 3);
      thread_current()->esp = myEsp;
      char* cmd_line = *(int*)myEsp;
      //if (!check_buffer (cmd_line)) {
      //  our_exit(-1);
      //}
      check_addr(cmd_line);
      check_addr(cmd_line + 3);
      check_buffer(cmd_line);
      if(!cmd_line) {
        our_exit(-1);
      } else {
        f->eax = -1;
        if(cmd_line) {
          //do exec ]
          tid = process_execute(cmd_line);
          //find child process
          struct list_elem *e;
          for (e = list_begin(&thread_current()->child_list); e != list_end(&thread_current()->child_list);
              e = list_next(e)) {
            struct thread* child = list_entry (e, struct thread, child_elem);
            if(child->tid == tid) {
              sema_down(&child->load_wait);
              if(child->loaded) {
                f->eax = tid;
              }
              break;
            }
          }
        }
      }
      break; 
    case SYS_WAIT:  /* Wait for a child process to die. */
      //printf("\nSysWait\n");
      myEsp += 4;
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      tid = *(tid_t*)myEsp;
      if(tid < 0) {
        our_exit(-1);
      } else {
        f->eax = process_wait(tid);
      }
      break; 
    case SYS_CREATE: 
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      file = *(int*)myEsp; //*(int*)myEsp
      check_addr(file);
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      size = *(unsigned*)myEsp;
      success = false;
      // valid arguments
      if(&size && file){ 
        lock_acquire(&file_lock);
        success = filesys_create(file, size);
        lock_release(&file_lock);
      } else {
        our_exit(-1);
      }
      f->eax = success;
      break; 
    case SYS_REMOVE: 
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      file = *(int*)myEsp;
      success = false;
      // valid argument
      if (file){
        lock_acquire(&file_lock);
        success = filesys_remove(file);
        lock_release(&file_lock);
      } else {
        our_exit(-1);
      }
      f->eax = success;
      break; 
    case SYS_OPEN: 
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      file = *(int*)myEsp;
      check_addr(file);
      if(!file) {
        our_exit(-1);
      }
      else {
        f->eax = -1;
        lock_acquire(&file_lock);
        struct file *success = filesys_open(file);
        lock_release(&file_lock);
        // file successfully opened 
        
        if (success){
          //Initialize open_files (if needed)
          //if(thread_current()->file_list == NULL) {
            //thread_current()->open_files = (struct file**) palloc_get_page(0);
          //}
          for(int i = 0; i < 128; i++) {
            if(thread_current()->file_list[i] == NULL) {
              thread_current()->file_list[i] = success;
              f->eax = i + 2;
              break;
            }
          }
        } 
      }
      break; 
    case SYS_FILESIZE: 
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      fd = *(int*)myEsp; 
      f->eax = filesize(fd);
      break; 
    case SYS_READ:  
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      fd = *(int*)myEsp;
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      buffer = *(int*)myEsp;
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      size = *(unsigned*)myEsp;
      f->eax = read(fd, buffer, size);
      break; 
    case SYS_WRITE: 
      myEsp += 4;
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      fd = *(int*)myEsp;
      myEsp += 4;
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      buffer = *(int*)myEsp;
      check_addr(buffer);
      myEsp += 4;
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      size = *(unsigned int*)myEsp;
      f->eax = -1;
      //if (!check_buffer (buffer)) {
      //  our_exit(-1);
      //}
      check_buffer(buffer);
      // check args
      if (&fd && fd > 0 && fd <= 130 && &size && buffer){
        if (fd == 1){
          putbuf(buffer, size);
          f->eax = size;
        } else {
          struct file* to_write = thread_current()->file_list[fd - 2];
          if(to_write) {
            lock_acquire(&file_lock);
            f->eax = file_write(to_write, buffer, size);
            lock_release(&file_lock);
          } else {
            our_exit(-1);
          }
        }
      } else {
        our_exit(-1);
      }
      break; 
    case SYS_SEEK:  
      //get params
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      fd = *(int*)myEsp;
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      unsigned position = *(int*)myEsp;
      seek(fd, position);
      break; 
    case SYS_TELL:  
      //Get params
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      fd = *(int*)myEsp;
      f->eax = tell(fd);
      break; 
    case SYS_CLOSE: 
      myEsp += sizeof(int);
      check_addr(myEsp);
      thread_current()->esp = myEsp;
      fd = *(int*)myEsp;
      close(fd);
      break; 
    default:
      our_exit(-1);
  }
}

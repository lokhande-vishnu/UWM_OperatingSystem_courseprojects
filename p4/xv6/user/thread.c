#include "types.h"
#include "x86.h"
#include "user.h"

#define PGSIZE (4096)

int
thread_create(void (*start_routine)(void*), void *arg)
{
  void *stack = malloc(2*PGSIZE);
  memset(stack, 0, 2*PGSIZE);
  if(stack == NULL)
    return -1;
  
  if((uint)stack % PGSIZE != 0) {
    void *aligned = stack + (PGSIZE - (uint)stack % 4096);
    stack = aligned;
  }
  return clone(start_routine, arg, stack);
}

int
thread_join()
{
  void *stack;
  int pid = join(&stack);
  free(stack);
  return pid;
}

#include "types.h"
#include "x86.h"
#include "user.h"

void
lock_init(lock_t *lock)
{
  lock->flag = 0;
}

void
lock_acquire(lock_t *lock)
{
  while(xchg(&lock -> flag, 1) == 1);
}

void
lock_release(lock_t *lock)
{
  lock->flag = 0;
}

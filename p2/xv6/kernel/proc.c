#define _PSENUM_ 0

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"

#define QLEN 100

int front = 0;
int rear = QLEN-1;
int size = 0;
int p3q[QLEN];

void addq(int pid) {
  rear = (rear + 1) % QLEN;
  size = size + 1;
  p3q[rear] = pid;
}

void remq(void) {
  front = (front + 1) % QLEN;
  size = size - 1;
}

int isemp(void) {
  return (size == 0);
}

int frontq(void) {
  if (isemp()) {
    return -1;
  }

  return p3q[front];
}

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);



// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.


  void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

  static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack if possible.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// Set up first user process.
  void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  acquire(&ptable.lock);
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
  int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
  int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  pid = np->pid;
  np->state = RUNNABLE;
  np->priority = 0;
  np->ticks[0] = 0;
  np->ticks[1] = 0;
  np->ticks[2] = 0;
  np->ticks[3] = 0;
  np->slice[0] = 0;
  np->slice[1] = 0;
  np->slice[2] = 0;
  np->slice[3] = 0;
  np->starved = 1;
  np->p3ticks = 0;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
  void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  iput(proc->cwd);
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a ch ild process to exit and return its pid.
// Return -1 if this process has no children.
  int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
  void
scheduler(void)
{
  struct proc *p;
  //int p3which = -1;

  for(;;){
    // Enable interrupts on this processor.
    sti();
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;
      
   
      /*      if( !(ticks%100) ) {
              bump_priority();
              }*/

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      // Process is done running for now.
      // It should have changed its p->state before coming back.
      //proc = 0;



      if (p->priority == 0) {


        if((p->slice[0] >= 5)){
          p->priority = 1;
          continue;
        }

        //cprintf("%d,%d,%d,%d\n", ticks, p->pid, p->priority, p->ticks[p->priority]);
        proc = p;
        switchuvm(p);
        p->state = RUNNING;
        p->starved = 0;
        p->ts = ticks;
        swtch(&cpu->scheduler, proc->context);
        p->ticks[0]++;
        p->slice[0]++;
        switchkvm();

        break;  

      }else if (p->priority == 1) {

        if((p->slice[1] >= 5)){
          p->priority = 2;
          continue;
        }

        //cprintf("%d,%d,%d,%d\n", ticks, p->pid, p->priority, p->ticks[p->priority]);
        proc = p;
        switchuvm(p);
        p->state = RUNNING;
        p->starved = 0;
        p->ts = ticks;
        swtch(&cpu->scheduler, proc->context);
        p->ticks[1]++;
        p->slice[1]++;
        switchkvm();


      } else if(p->priority == 2) {


        if((p->slice[2] >= 10)){
          p->priority = 3;
          continue;
        }

        //cprintf("%d,%d,%d,%d\n", ticks, p->pid, p->priority, p->ticks[p->priority]);
        proc = p;
        switchuvm(p);
        p->state = RUNNING;
        p->starved = 0;
        p->ts = ticks;
        swtch(&cpu->scheduler, proc->context);
        p->ticks[2]++;
        p->slice[2]++;
        switchkvm();


      } else if(p->priority == 3) {

        addq(p->pid);

        if (p->pid == frontq()) {
          //cprintf("%d,%d,%d,%d\n", ticks, p->pid, p->priority, p->ticks[p->priority]);
          proc = p;
          switchuvm(p);
          p->state = RUNNING;
          p->starved = 0;
          p->ts = ticks;
          swtch(&cpu->scheduler, proc->context);
          p->ticks[3]++;
          p->slice[3]++;
          p->p3ticks++;
          switchkvm();

          if ((p->p3ticks != 0) && (((p->p3ticks) % 20) == 0)) {
            remq();
            p->p3ticks = 0;

          }
        }else {

          continue;

        }

      }


    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
  void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
  void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
  void
forkret(void)
{
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
  void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
  static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
  void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
  int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
  void
procdump(void)
{
  static char *states[] = {
    [UNUSED]    "unused",
    [EMBRYO]    "embryo",
    [SLEEPING]  "sleep ",
    [RUNNABLE]  "runble",
    [RUNNING]   "run   ",
    [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

  int
sys_getpinfo(void)
{
  //TODO : implimentation of getpinfo added here 
  struct proc *p;
  char *ps;
  if(argptr(0, &ps, sizeof(struct pstat)) < 0){
    return -1;
  }

  // we have the pointer decoded
  // I am going to go through the ptable one process at a time and fill 
  // the pointer location
  acquire(&ptable.lock);
  int i = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    //if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
    //if(p->state == SLEEPING){
    //}

    // Filling up the table
    ((struct pstat *)ps)->inuse[i] = 1;
    ((struct pstat *)ps)->pid[i] = p->pid;
    ((struct pstat *)ps)->priority[i] = p->priority;
    ((struct pstat *)ps)->state[i] = p->state;
    int j = 0;
    for(j=0;j<4;j++){
      ((struct pstat *)ps)->ticks[i][j] = p->ticks[j];
    }
    i++;
  }
  release(&ptable.lock);
  return 0;
}

  void
bump_priority(void)
{

  // TODO : implimentation added here 
  // priority bump up
  struct proc *p;
  acquire(&ptable.lock);  //DOC: yieldlock
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE)
      continue;


    if((p->starved) == 1){
      switch (p->priority) {

        case 1:
        case 2:
          p->priority--;
          p->slice[p->priority] = 0;
          break;
        case 3:
          p->priority--;
          p->slice[p->priority] = 0;
          front = 0;
          rear = QLEN-1;
          size = 0;
          break;
        case 0:
        default:
          break;  
      }
    } else {
      p->starved = 1;
    }
  }
  release(&ptable.lock);
}



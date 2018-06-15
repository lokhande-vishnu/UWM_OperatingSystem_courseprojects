#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"
#define check(exp, msg) if(exp) {} else {				\
  printf(1, "%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg); \
  exit();}

int workload(int n) {
  int i, j = 0;
  for(i = 0; i < n; i++)
    j += i * j + 1;
  return j;
}

  int
main(int argc, char *argv[])
{
  struct pstat st;

  sleep(10);
  int i = workload(50000000);

  int pid1;
  int pid2 = getpid();

  int parent_tick = -1;
  int child_priority = 4, child_tick = -1;

  if (fork() == 0) {
    pid1 = getpid(); 

    while (child_priority != 3) {
      check(getpinfo(&st) == 0, "getpinfo");
      for(i = 0; i < NPROC; i++) {
        if (st.inuse[i]) {
          // slot corresponding to the child process
          if (pid1 == st.pid[i]) {
            child_priority = st.priority[i];
            child_tick = st.ticks[i][child_priority];

          } else if (pid2 == st.pid[i]) {
            if (parent_tick == -1) {
              parent_tick = st.ticks[i][3];

            } else if (child_priority < 3) {
              // slot corresponding to the parent process
              if (parent_tick != st.ticks[i][3]) {
                printf(1, "parent: previous %d ", parent_tick);
                printf(1, "current %d ticks\n", st.ticks[i][3]);
                printf(1, "child: runs for %d ", child_tick);
                printf(1, "ticks on level %d\n\n", child_priority);
              }
              check(parent_tick == st.ticks[i][3], 
                  "Parent process should not run until child process is in the lowest level");
            }
          }
        }
      }
    }
    printf(1, "TEST PASSED");
  } else {
    wait();
  }

  exit();
}

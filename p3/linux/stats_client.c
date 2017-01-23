#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <semaphore.h>
#include <fcntl.h>
#include "stats.h"

key_t key = -99;

#define BILLION 1E9

struct timespec start, end;

void intHandler() {
  stats_unlink(key);
  _exit(0);
}

void usage() {
  fprintf(stderr,
      "Usage: stats_client -k key -p priority -s sleeptime_ns -c cputime_ns\n");
  exit(1);
}

void spinwait_ns(double nsecs) {
  struct timespec rettime, now;
  double Rettime, Now;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &rettime);
  Rettime = rettime.tv_sec * BILLION + rettime.tv_nsec + nsecs;
  while (1) {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
    Now = now.tv_sec * BILLION + now.tv_nsec;
    if (Now > Rettime)
      break;
  }
}


int main(int argc, char* argv[]) {
  struct sigaction sa_cuser;
  sa_cuser.sa_flags = 0;
  sa_cuser.sa_handler = &intHandler;
  sigaction(SIGINT, &sa_cuser, NULL);

  // Default values for the arguments
  int priority = 0;
  int sleeptime_ns = 500000000;
  double cputime_ns = 500000000;

  // Parsing the arguments
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "k:p:s:c:")) != -1) {
    switch (c) {
      case 'k':
        key  = (key_t) strtoul(optarg, NULL, 0);
        break;
      case 'p':
        priority = strtol(optarg, NULL, 10);
        break;
      case 's':
        sleeptime_ns = strtol(optarg, NULL, 10);
        break;
      case 'c':
        cputime_ns = strtod(optarg, NULL);
        break;
      default:
        usage();
    }
  }

  if (key == -99) {
    usage();
  }

  // Setting defualt values for the argumnets
  stats_t *addr = stats_init(key);
  if (addr == NULL) {
    perror("memfull\n");
    exit(1);
  }

  /*sem_t * mutex;
  if ((mutex = sem_open("/vsr1912_sem", 0)) == SEM_FAILED) {
    perror("Creating semaphore in Server failed\n");
    exit(1);
    }*/

  int which = PRIO_PROCESS;
  // sem_wait(mutex);
  addr->pid = getpid();
  strncpy(addr->pname, argv[0], 15);
  addr->counter = 0;
  addr->cpu_secs = 0;
  if (setpriority(which, addr->pid, priority) == -1) {
    fprintf(stderr, "error in set priority\n");
    exit(1);
  }
  addr->priority = getpriority(which, addr->pid);
  // sem_post(mutex);

  // Sleeping and Displaying the values
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
  while (1) {
    // Spin Waiting
    spinwait_ns(cputime_ns);

    // Sleeping
    struct timespec tim;
    tim.tv_sec = 0;
    tim.tv_nsec = sleeptime_ns;
    if (nanosleep(&tim , NULL) < 0) {
      perror("Nano sleep system call failed \n");
      exit(1);
    }

    // Updating the statistics
    // sem_wait(mutex);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
    addr->cpu_secs = (end.tv_sec - start.tv_sec)
          + (end.tv_nsec - start.tv_nsec)/BILLION;
    addr->counter = addr->counter + 1;
    addr->priority = getpriority(which, addr->pid);
    // sem_post(mutex);
  }
  return 0;
}

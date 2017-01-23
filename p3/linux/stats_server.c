#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include "stats.h"

void *mem;
int shmid = 0;

void usage() {
  fprintf(stderr, "Usage: stat_server -k key \n");
  exit(1);
}

void intHandler() {
  if (shmctl(shmid, IPC_RMID, NULL) == -1)
    fprintf(stderr, "ERROR: Destroying shared memory segment failed\n");
  if (sem_unlink("/vsr1678_sem") == -1)
    fprintf(stderr, "ERROR: Destroying semaphore in server failed\n");
  exit(0);
}

int main(int argc, char* argv[]) {
  struct sigaction sa_suser;
  sa_suser.sa_flags = 0;
  sa_suser.sa_handler = &intHandler;
  sigaction(SIGINT, &sa_suser, NULL);

  // Create a semaphore used as mutex
  sem_t * mutex;
  if ((mutex = sem_open("/vsr1678_sem", O_CREAT, 0644, 1)) == SEM_FAILED) {
    perror("Creating semaphore in Server failed\n");
    exit(1);
  }



  key_t key;
  ssize_t size = (ssize_t)sysconf(_SC_PAGESIZE);

  // Parsing the arguments
  int c = 0;
  opterr = 0;
  while ((c = getopt(argc, argv, "k:")) != -1) {
    switch (c) {
      case 'k':
        key  = (key_t) strtoul(optarg, NULL, 0);
        break;
      default:
        usage();
    }
  }

  // Creating shared memory
  if ((shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | SHM_R | SHM_W)) == -1) {
    perror("shmget");
    exit(1);
  }

  // Attach the created shared memory segement to the server's memory space
  mem = shmat(shmid, NULL, 0);
  if (mem == (void *)(-1)) {
    perror("shmat");
    exit(1);
  }
  memset(mem, '\0', size);


  // Initialize the shared memory segment to be  16 * stats_t
  int i = 0;
  stats_t *stats_list[16] = {NULL};
  // sem_wait(mutex);
  for (i = 0; i < 16; i++) {
    stats_list[i] = (stats_t*) (mem + sizeof(stats_t) * i);
    stats_list[i]->pid = -1;
  }
  // sem_post(mutex);
  // sem_close(mutex);

  // Sleeping and Displaying the values
  int iter = 0;
  while (1) {
    sleep(1);

    for (i = 0; i < 16; i++) {
      if (stats_list[i]->pid != -1) {
        fprintf(stdout, "%d ", iter);
        fprintf(stdout, "%d ", stats_list[i]->pid);
        fprintf(stdout, "%.15s ", stats_list[i]->pname);
        fprintf(stdout, "%d ", stats_list[i]->counter);
        fprintf(stdout, "%.2f ", stats_list[i]->cpu_secs);
        fprintf(stdout, "%d\n", stats_list[i]->priority);
      }
    }
    iter++;
    fprintf(stdout, "\n");
  }
  return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include "stats.h"

stats_t*
stats_init(key_t key) {
  ssize_t size = (ssize_t)sysconf(_SC_PAGESIZE);
  stats_t *addr = NULL;
  int clientid = 0;


  // grab the mutex/ semaphore
  sem_t* mutex;
  if ((mutex = sem_open("/vsr1678_sem", 0)) == SEM_FAILED) {
    perror("Accessing semaphore in client failed\n");
    exit(1);
  }


  // Attach to the shared memory if it exists
  if ((clientid = shmget(key, size, SHM_R | SHM_W)) == -1) {
    perror("in shared memory of client does not exist");
    exit(1);
  }

  // Acquiring pointer to shared memory
  char *mem = (char *)shmat(clientid, NULL, 0);
  if (mem == (void *)(-1)) {
    perror("shmat in client");
    exit(1);
  }


  sem_wait(mutex);
  // loop through list to find the first empty slot
  int i = 0;
  for (i = 0; i < 16; i++) {
    if (((stats_t*) (mem  + sizeof(stats_t) * i)) -> pid == -1) {
      addr =  (stats_t*)( mem + sizeof(stats_t) * i);
      sem_post(mutex);
      sem_close(mutex);
      return(addr);
    }
  }

  // All 16 slots are allocated at this point
  sem_post(mutex);
  sem_close(mutex);
  return(NULL);
}

int
stats_unlink(key_t key) {
  ssize_t size = (ssize_t)sysconf(_SC_PAGESIZE);
  int clientid = 0;

  // grab the mutex/ semaphore
  sem_t* mutex;
  if ((mutex= sem_open("/vsr1678_sem", 0)) == SEM_FAILED) {
    perror("Accessing semaphore in client failed\n");
    exit(1);
  }



  // Attach to the shared memory if it exists
  if ((clientid = shmget(key, size, SHM_R | SHM_W)) == -1) {
    perror("un shared memory of client does not exist");
    exit(1);
  }

  // Acquiring pointer to shared memory
  char *mem = (char *)shmat(clientid, NULL, 0);
  if (mem == (void *)(-1)) {
    perror("shmat in client");
    exit(1);
  }

  // Get the current process's pid
  int curr_pid = getpid();

  sem_wait(mutex);
  // loop through list to find the slot with the matching pid
  int i = 0;
  for (i = 0; i < 16; i++) {
    if (((stats_t*) (mem + sizeof(stats_t) * i)) -> pid == curr_pid) {
      ((stats_t*) (mem + sizeof(stats_t) * i)) ->pid = -1;
      ((stats_t*) (mem + sizeof(stats_t) * i)) ->counter = 0;
      ((stats_t*) (mem + sizeof(stats_t) * i)) ->priority = 0;
      ((stats_t*) (mem + sizeof(stats_t) * i)) ->cpu_secs = 0;

      // detach from the shared memory segment
      if (shmdt(mem) == -1) {
        perror("shmdt error\n");
        exit(1);
      }
      mem = NULL;
      sem_post(mutex);
      sem_close(mutex);
      return(0);
    }
  }
  // Cannot find the segment associated with the current pid
  sem_post(mutex);
  sem_close(mutex);
  return(-1);
}


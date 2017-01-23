#include <sys/shm.h>

typedef struct {
  // You may add any new fields that you believe are necessary
  int pid;        // Do not remove or change
  int counter;    // Do not remove or change
  int priority;   // Do not remove or change
  double cpu_secs; // Do not remove or change
  char pname[15];
  // You may add any new fields that you believe are necessary
} stats_t;

stats_t* stats_init(key_t key);
int stats_unlink(key_t key);

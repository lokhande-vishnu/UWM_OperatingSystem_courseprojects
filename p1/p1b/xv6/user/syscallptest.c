#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char *argv[])
{
  int i = 0, first_call = 0, last_call = 0;
  int N = atoi(argv[1]); 
  first_call = getnumsyscallp();
  while (i < N) {
    getpid();
    i++;
  }
  last_call = getnumsyscallp();
  printf(1,"%d\n%d\n",first_call,last_call); // printing the calls
  exit();
}

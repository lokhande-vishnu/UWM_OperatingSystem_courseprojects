#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char *argv[])
{
  // extract the first argument
  unsigned long long int N;
  unsigned long long int x = 0;
  N = atoi(argv[1]);
  unsigned long long int i = 0;
  for (i = 0; i < N; i++) {
    //make N system calls
    //lets just do getpid

    x++;
  }
  exit();
  return x ;
}

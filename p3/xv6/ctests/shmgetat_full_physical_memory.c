#include "types.h"
#include "stat.h"
#include "user.h"

void
test_failed()
{
	printf(1, "TEST FAILED\n");
	exit();
}

void
test_passed()
{
 printf(1, "TEST PASSED\n");
 exit();
}

int
main(int argc, char *argv[])
{
  int pid;

  pid = fork();
  if (pid < 0) {
    test_failed();
  }
  else if (pid == 0) {
    char *ptr;
    int count = 0;
    while (ptr != (char *) -1) {
      ptr = sbrk(4096);
      count++;
    }

    while (count != 1) { //bunch of sbrks called
      pid = fork();
      if (pid < 0) {
        printf(1, "fork failed = %d\n", pid);
        printf(1, "pid = %d\n", getpid());

        printf(1, "physical mem should be full %d\n", count);
        char *p = (char *) shmgetat(3, 1);
        printf(1, "p = %d\n", p);

        test_failed();
      }
      else if (pid == 0) {
        while (ptr != (char *) -1) {
          ptr = sbrk(4096);
          count++;
        }
      }
      else {
        printf(1, "waiting on %d\n", pid);
        wait();
      }
    }
  }
  else {
    wait();
  }

	test_passed();
	exit();
}

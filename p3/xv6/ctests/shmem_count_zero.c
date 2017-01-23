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
 	int n;
  int i;

  for (i = 0; i < 8; i++) {
    n = shm_refcount(i);
    if (n != 0) {
      test_failed();
    }
  }

	test_passed();
	exit();
}

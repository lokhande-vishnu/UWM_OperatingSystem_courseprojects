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
 	char *ptr;
	int i;

  for (i = 3; i >= 0; i--) {
    ptr = (char *) shmgetat(i, 1);
    if (ptr == (char *) -1) {
      test_failed();
    }

    *ptr = 'c' + i;
  }

	test_passed();
	exit();
}

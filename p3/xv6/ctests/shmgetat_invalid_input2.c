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
  void *ptr;

	ptr = (char *) shmgetat(0, 2);
	if (ptr == (char *) -1) {
		test_failed();
	}

  ptr = (char *) shmgetat(0, 200);
	if (ptr == (char *) -1) {
		test_failed();
	}

  ptr = (char *) shmgetat(1, 3);
  if (ptr == (char *) -1) {
    test_failed();
  }

  ptr = (char *) shmgetat(1, -200);
  if (ptr == (char *) -1) {
    test_failed();
  }

	test_passed();
	exit();
}

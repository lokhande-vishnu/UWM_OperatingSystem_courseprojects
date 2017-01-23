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

	ptr = (char *) shmgetat(-1, 1);
	if (ptr != (char *) -1) {
		test_failed();
	}

	ptr = (char *) shmgetat(-100, 1);
	if (ptr != (char *) -1) {
		test_failed();
	}

	ptr = (char *) shmgetat(8, 1);
	if (ptr != (char *) -1) {
		test_failed();
	}

	ptr = (char *) shmgetat(100, 1);
	if (ptr != (char *) -1) {
		test_failed();
	}

  //TODO(arman): test num_pages

	test_passed();
	exit();
}

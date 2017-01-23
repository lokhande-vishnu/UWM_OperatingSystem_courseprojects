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
	void *ptr2;

	ptr = (char *) shmgetat(3, 1);
	if (ptr == (char *) -1) {
		test_failed();
	}

	ptr2 = (char *) shmgetat(3, 1);
	if (ptr == (char *) -1) {
		test_failed();
	}

	if (ptr != ptr2) {
		test_failed();
	}

	test_passed();
	exit();
}

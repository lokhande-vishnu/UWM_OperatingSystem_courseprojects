#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

#define PGSIZE 4096

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

	ptr = (char *) shmgetat(4, 1);
	if (ptr == (char *) -1) {
		test_failed();
	}

  if (ptr != (char *) USERTOP-PGSIZE) {
    test_failed();
  }

	ptr2 = (char *) shmgetat(4, 2);
	if (ptr == (char *) -1) {
		test_failed();
	}

	if (ptr != ptr2) {
		test_failed();
	}

	test_passed();
	exit();
}

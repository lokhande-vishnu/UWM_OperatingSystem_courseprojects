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
 	int n;

	int i;
	for (i = 0; i < 8; i++) {
		ptr = (char *) shmgetat(i, 2);
		if (ptr == (char *) -1) {
			test_failed();
		}
		n = shm_refcount(i);
		if (n != 1) {
			test_failed();
		}
	}

	test_passed();
	exit();
}

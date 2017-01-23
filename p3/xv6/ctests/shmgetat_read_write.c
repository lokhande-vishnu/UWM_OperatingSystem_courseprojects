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

	for (i = 0; i < 4; i++) {
		ptr = (char *) shmgetat(i, 1);
		if (ptr == (char *) -1) {
			test_failed();
		}

		char c = 'c';
		ptr[100] = c; //write

		char d = *(ptr+100); //read

		if (c != d) {
			test_failed();
		}
	}

	test_passed();
	exit();
}

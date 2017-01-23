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
	char arr[6] = "CS537";


	int pid = fork();
	if (pid < 0) {
		test_failed();
	}
	else if (pid == 0) {
		ptr = (char *) shmgetat(3, 1);
		if (ptr == (char *) -1) {
			test_failed();
		}

		for (i = 0; i < 5; i++) {
			*(ptr+i) = arr[i];
		}

		exit();
	}
	else {
		wait();

		ptr = (char *) shmgetat(3, 1);
		if (ptr == (char *) -1) {
			test_failed();
		}

		for (i = 0; i < 5; i++) {
			if (*(ptr+i) != arr[i]) {
				test_failed();
			}
		}
	}

	test_passed();
	exit();
}

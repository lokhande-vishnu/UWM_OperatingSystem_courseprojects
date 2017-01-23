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

	ptr = (char *) shmgetat(2, 1);
	if (ptr == (char *) -1) {
		test_failed();
	}

	int pid = fork();
	if (pid < 0) {
		test_failed();
	}
	else if (pid == 0) {
		sleep(20);

		for (i = 0; i < 5; i++) {
			if (*(ptr+i) != arr[i]) {
				test_failed();
			}
		}
	}
	else {
		for (i = 0; i < 5; i++) {
			*(ptr+i) = arr[i];
		}

		wait();
	}

	test_passed();
	exit();
}

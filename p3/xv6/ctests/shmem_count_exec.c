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

char *args[] = { "echo", 0 };

int
main(int argc, char *argv[])
{
	void *ptr;
 	int n;
	int i;

	for (i = 0; i < 4; i++) {
		ptr = (char *) shmgetat(i, 1);
		if (ptr == (char *) -1) {
			test_failed();
		}
	}

  for (i = 0; i < 4; i++) {
    n = shm_refcount(i);
    if (n != 1) {
      test_failed();
    }
  }

	int pid = fork();
	if (pid < 0) {
		test_failed();
	}
	else if (pid == 0) {
		for (i = 0; i < 4; i++) {
			n = shm_refcount(i);
			if (n != 2) {
				test_failed();
			}
		}

    exec("echo", args); //echo represents shmem_count_exec_helper.c
    printf(1, "exec failed!\n");
    test_failed();
		exit();
	}
	else {
		wait();
	}

	test_passed();
	exit();
}

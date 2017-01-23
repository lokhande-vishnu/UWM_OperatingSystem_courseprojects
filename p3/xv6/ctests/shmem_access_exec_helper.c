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
 	char *ptr = (char *)(USERTOP - 4096);
  *ptr = 'c';
	test_failed();
	exit();
}

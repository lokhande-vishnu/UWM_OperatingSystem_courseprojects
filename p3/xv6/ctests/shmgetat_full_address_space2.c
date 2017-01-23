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
  ptr = (char *) shmgetat(0, 1);
  if (ptr == (char *) -1) {
    test_failed();
  }
  
  void *ptr2;
  while (1) {
    ptr2 = sbrk(1024);
    if (ptr2 >= ptr && ptr2 != (char *) -1) {
      test_failed();
    }
    
    if (ptr2 == (char *) -1) {
      break;
    }
  }
  
  test_passed();
  exit();
}

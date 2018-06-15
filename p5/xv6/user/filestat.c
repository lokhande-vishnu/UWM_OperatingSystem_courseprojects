#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
  if(argc < 2){
    printf(1, "Usage: filestat pathname\n");
    exit();
  }
  
  int fd  = open((char*)argv[1], 0);
  struct stat fs;
  if(fstat(fd, &fs) < 0){
    printf(1, "Error retriving file stats.\n.");
    exit();
  }

  if(fs.type == T_CHECKED){
    printf(1, "File Type: %d\tSize: %d\tChecksum: %d\n", fs.type, fs.size, fs.checksum);
  } else {
    printf(1, "File Type: %d\tSize: %d\n", fs.type, fs.size);
  }
  exit();
}

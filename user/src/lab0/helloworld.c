#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
  // Student code goes here
  const char* str = "Hello World\n";
  const int length = 12;
  int fd = open("helloworld.txt", O_WRONLY|O_CREATE);
  if (fd < 0){
    printf(1,"failed to create");
    exit();
  }

  int write_status = write(fd, str, length);

  if(write_status < 0){
    printf(1,"file failed to be written");
    close(fd);

    exit();
  }

fd = open("helloworld.txt", O_RDONLY);

if(fd < 0){
  printf(1,"failed to read");
  exit();
}

char helloworldbuff[length];

fd = read(fd, helloworldbuff, length);

if(fd < 0){
  printf(1, "failed to read");
  close(fd);
  exit();
}
  

//0 is stin
//1 is std out
//2 is std err
printf(1, helloworldbuff);

  exit();
}
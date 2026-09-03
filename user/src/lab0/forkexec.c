#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  // Student code goes here


  int pid = fork();

  if(pid==0){
    //we are in the child process
    exec("echo", argv);

  }
  else{
    //i am in the parent
    wait();
  }



  exit();
}
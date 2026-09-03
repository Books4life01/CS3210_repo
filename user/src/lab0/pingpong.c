#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  // Student code goes here

  //child <-> parent

  int p2c[2];
  int c2p[2];

  pipe(p2c);
  pipe(c2p);

  if(fork()==0){
    //receive parent pid

    close(p2c[1]);//child does not need to write to first pipe
    close(c2p[0]);//child does not ened to read from 2nd pipe

    //sened child pid
    char child_pid = getpid();

    char parent_pid;
    read(p2c[0], &parent_pid, 1);

    printf(1, "child: received %d from parent\n", parent_pid);

    write(c2p[1], &child_pid, 1);

    

close(p2c[0]);
    close(c2p[1]);

  }
  else{
    close(p2c[0]);
    close(c2p[1]);
    char parent_pid = getpid();
    write(p2c[1],&parent_pid, 1);
    char child_pid;
    read(c2p[0],&child_pid, 1);


    printf(1, "parent: received %d from child\n", child_pid);




    close(p2c[1]);
    close(c2p[0]);

    wait();
  }

  

  exit();
}
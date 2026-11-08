#include "asm/x86.h"
#include "types.h"
#include "defs.h"
#include "memlayout.h"

int sys_eth_send(void){
    char *data;
    int size;

    if(argint(2, &size)<0){
        return -1;
    }
    if(argptr(1,&data, size)<0){
        return -1;
    }
    return 0;
}


int sys_eth_receive(void){
    char *data;
    int size;

    if(argint(2, &size)<0){
        return -1;
    }
    if(argptr(1,&data, size)<0){
        return -1;
    }
    return 0;
}

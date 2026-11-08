#include "asm/x86.h"
#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "bpf.h"

//Initializes the BPF VM.

void bpf_init(void){
    return;
}
//Loads a BPF program into the VM.

int bpf_load(const union bpf_attr *attr){
    return 0;
}
//Runs the BPF program.

int bpf_run(char *pkt, uint pktsize){
    return 0;
}


int sys_bpf(){
    int cmd; 
    char*attr;
    if(argint(1,&cmd)<0){
        return -1;
    }
    if(argptr(2, &attr, sizeof(union bpf_attr))<0){
        return -1;
    }
    return 0;
    // (union bpf_attr *)attr
}

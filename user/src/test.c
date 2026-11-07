#include "types.h"
#include "stat.h"
#include "user.h"



int main(int arc, char* argv[]){
    lspci();
    unsigned char mac[6];
    whatsmymac(mac);
    for(int i = 0; i < 6; i++){
        printf(2, "%x ", mac[i]);
    }
    printf(1,"\n");
    exit();
}
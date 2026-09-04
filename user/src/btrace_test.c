#include "user.h"
#include "types.h"
void c(){
    backtrace();
}
void b(){
    c();
}



void a(){
    b();
}



int main(int argc, char *argv[]){
    a();
    return 0;
}




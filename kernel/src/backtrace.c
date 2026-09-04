#include "backtrace.h"
#include "asm/x86.h"
#include "stab.h"
#include "stdio.h"
void backtrace(){

//start at ebp
uint* ebp;
read_ebp(ebp);//ebp now contains the adress of the first base pointer

struct stab_info info;//do i need to malloc this
const char* func_name;///not null terminated so i need length to
 int name_length;


uint func_start_addr;
uint* return_address;
uint displacment;


//Stack grows  from high to low memoery addreses; so we are traversing from low to high

cprintf("Backtrace:\n");
//while current bp isnt the terminating bp
while (*ebp!=0xF00){

    //get associated return adress 
    return_address = (uint*)(ebp+1);//the return adress is stored one memory locaiton lower lower than the base pointer
    ebp = (uint*)*ebp;//the adress of the next base pointer is the value at ebp'


    //pass it to stab to get filename line number, addr start
    if (stab_info(*return_address, &info)!=0){
        func_name = "unknown";
        name_length = 8;
    }
    else{
        //retreive info about function
        func_name = info.eip_fn_name;
        name_length = info.eip_fn_namelen;
    }
    

    

    func_start_addr = info.eip_fn_addr;
    
    //calculate RA-addr and print to console
    displacment = (*return_address)-func_start_addr;


    cprintf("   <%p> %.*s+%d\n", *return_address, name_length, func_name, displacment);
}
}
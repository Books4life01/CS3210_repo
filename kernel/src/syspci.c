#include "asm/x86.h"
#include "types.h"
#include "defs.h"
#include "memlayout.h"

#define NUMBUSES 256 //s^8
#define NUMDEVICES 32 //s^5
#define CONFIGFUN 0
#define CONFIGADRESS 0xCF8
#define CONFIGDATA 0xCFC
#define VENDORMASK 0x0000FFFF
#define NUMFUNCTIONS 8//2^3
#define RALOFFSET 0x5400
#define RAHOFFSET 0x5404

int sys_whatsmymac(void){
    uint config;
    ushort deviceID;
    ushort vendorID; 
    uint32_t address;
    uint bar;
    char* mac;

    if(argptr(0, &mac, 6) < 0){
        return -1;
    }
   

    for(int b = 0; b < NUMBUSES; b++){
        for(int d = 0; d < NUMDEVICES; d++){
             address = (uint32_t)((b << 16) | (d << 11) |(0 << 8) | (0 & 0xFC) | (1U<<31));
            
            //put the adress in xCf8
            outl(CONFIGADRESS, address);
            //read the config back in
            config = inl(CONFIGDATA);
            deviceID = (short)(config>>16);
            vendorID = (short)(config&VENDORMASK);
            if(vendorID!=0x8086 || deviceID != 0x100e){
                continue;
            }

            //read the bar
            address = (uint32_t)((b << 16) | (d << 11) |(0 << 8) | ((0x4<<2) & 0xFC) | (1U<<31));
            outl(CONFIGADRESS, address);
            bar = inl(CONFIGDATA);
            bar&=0xFFFFFFF0;

            //Set the Memory space enable in teh command register
            address = (uint32_t)((b << 16) | (d << 11) |(0 << 8) | ((0x1<<2) & 0xFC) | (1U<<31));
            outl(CONFIGADRESS, address);//set teh adress
            uint command_reg = inl(CONFIGDATA);//read teh data
            command_reg|=0x00000002;//set memory mapped acess mdoe on
            outl(CONFIGDATA, command_reg);//write the data
        }
    }
    uint* base = (uint*)bar;
    //RAL holds lower 32 bits
    //RAH holds upper 16 bits

    mac[0] = base[RALOFFSET>>2]&0xFF;
    mac[1] = (base[RALOFFSET>>2]>>8)&0xFF;
    mac[2] = (base[RALOFFSET>>2]>>16)&0xFF; 
    mac[3] = (base[RALOFFSET>>2]>>24)&0xFF;
    mac[4] = (base[RAHOFFSET>>2])&0xFF;
    mac[5] = (base[RAHOFFSET>>2]>>8)&0xFF;

    return 0;
}
int sys_lspci(void){
    



    uint config;
    ushort deviceID;
    ushort vendorID;
    uint header;
    uint32_t address;
    short numFunctions;
    for(int b = 0; b < NUMBUSES; b++){
        for(int d = 0; d < NUMDEVICES; d++){
            //check to see if multi function
            address = (uint32_t)((b << 16) | (d << 11) |(0 << 8) | ((0x3<<2) & 0xFC) | (1U<<31));
            outl(CONFIGADRESS, address);
            header = inl(CONFIGDATA);
            if(((header>>16)& 0x80)==0){//this is not a multi function 
                numFunctions = 1;
            }
            else{
                numFunctions = NUMFUNCTIONS;
            }
            
            for(int f = 0; f < numFunctions; f++){
            //construct the config adress, bus adress is bits 23-16, device is bits 15-11, the funciton for config is always 0, the offset for the device and vendor id is also 0, the enable bit should be set at bit 31
             address = (uint32_t)((b << 16) | (d << 11) |
              (f << 8) | (0 & 0xFC) | (1U<<31));
            
            //put the adress in xCf8
            outl(CONFIGADRESS, address);

            //read the config back in
            config = inl(CONFIGDATA);

            

            deviceID = (short)(config>>16);
            vendorID = (short)(config&VENDORMASK);
            if(vendorID==0xFFFF || deviceID == 0xFFFF){
                continue;
            }

            //hte upper 16 bits are devide id, the lower 16 bits are vendor id
            cprintf("Vendor %x Device %x\n", vendorID, deviceID);
            }
        }
    }


    return 0;
}
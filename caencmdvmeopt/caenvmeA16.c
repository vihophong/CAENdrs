void init_nbbqvio();
unsigned short vme_read16(unsigned int addr);
unsigned int vme_read32(unsigned int addr);
int vme_write16(unsigned int addr,unsigned short sval);
int vme_write32(unsigned int addr,unsigned int lval);
int vme_amsr(unsigned int lval);
void release_nbbqvio();

#include "CAENVMEtypes.h"
#include "CAENVMElib.h"

void init_nbbqvio(){
  
  CVBoardTypes VMEBoard = cvV2718;
  short Link = 1;
  short Device = 0;

  if( CAENVME_Init(VMEBoard, Link, Device, &BHandle) != cvSuccess )
    {
      printf("\n\n Error opening the device\n");
      exit(1);
    }
  
}

unsigned short vme_read16(unsigned int addr){
  short Data;
  CAENVME_ReadCycle(BHandle,addr,&Data,cvA32_U_DATA,cvD16);
  return Data;
}

unsigned int vme_read32(unsigned int addr){
  int Data;
  CAENVME_ReadCycle(BHandle,addr,&Data,cvA32_U_DATA,cvD32);
  return Data;
}

int vme_write16(unsigned int addr,unsigned short sval){
  CAENVME_WriteCycle(BHandle,addr,&sval,cvA32_U_DATA,cvD16);
  return 0;
}

int vme_write16a16(unsigned int addr,unsigned short sval){
  CAENVME_WriteCycle(BHandle,addr,&sval,cvA16_U,cvD16);
  return 0;
}

int vme_write32(unsigned int addr,unsigned int lval){
  CAENVME_WriteCycle(BHandle,addr,&lval,cvA32_U_DATA,cvD32);
  return 0;
}

int vme_amsr(unsigned int lval){
  //am = val;
  return 0;
}

void release_nbbqvio(){
  CAENVME_End(BHandle);
}

int vread32(unsigned int addr, int *val){
  *val = vme_read32(addr);
  return *val;
}

short vread16(unsigned int addr, short *val){
  *val = vme_read16(addr);
  return *val;
}

void vwrite32(unsigned int addr, int *val){
  vme_write32(addr, *val);
}

void vwrite16(unsigned int addr, short *val){
  vme_write16(addr, *val);
}

void vwrite16a16(unsigned int addr, short *val){
  vme_write16a16(addr, *val);
}


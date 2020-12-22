#include <CAENVMEtypes.h>
#include <CAENVMElib.h>
#include "libbbcaenvme.h"

static int BHandle = -1;
static CVAddressModifier AM = cvA32_U_DATA;
static unsigned int irqmask;

int init_caen(CVBoardTypes VMEBoard, short Link, short Device){
  if( CAENVME_Init(VMEBoard, Link, Device, &BHandle) != cvSuccess ) {
    printf("\n\n Error opening the device\n");
    return 0;
  }
  return 1;
}

int init_caen_v2718(void){
  return init_caen(cvV2718, 0, 0);
}

int init_caen_v1718(void){
  return init_caen(cvV1718, 0, 0);
}

unsigned short vme_read16(unsigned int addr){
  short Data;
  CAENVME_ReadCycle(BHandle,addr,&Data, AM, cvD16);
  return Data;
}

unsigned int vme_read32(unsigned int addr){
  int Data;
  CAENVME_ReadCycle(BHandle,addr,&Data, AM, cvD32);
  return Data;
}

int vme_write16(unsigned int addr,unsigned short sval){
  CAENVME_WriteCycle(BHandle,addr,&sval, AM, cvD16);
  return 0;
}


int vme_write32(unsigned int addr,unsigned int lval){
  CAENVME_WriteCycle(BHandle,addr,&lval, AM,cvD32);
  return 0;
}

int vme_amsr(unsigned int lval){
  switch(lval){
  case 0x2D:
    AM = cvA16_S;
    break;
  case 0x29:
    AM = cvA16_U;
    break;
  case 0x3F:
    AM = cvA24_S_BLT;
    break;
  case 0x3D:
    AM = cvA24_S_DATA;
    break;
  case 0x3B:
    AM = cvA24_U_BLT;
    break;
  case 0x39:
    AM = cvA24_U_DATA;
    break;
  case 0x0F:
    AM = cvA32_S_BLT;
    break;
  case 0x0D:
    AM = cvA32_S_DATA;
    break;
  case 0x0B:
    AM = cvA32_U_BLT;
    break;
  case 0x09:
    AM = cvA32_U_DATA;
    break;
  default:
    AM = cvA32_U_DATA;
  }
  return 0;
}

void release_caen(){
  CAENVME_End(BHandle);
  BHandle = -1;
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

void vme_define_intlevel(int level){
  irqmask = (1<<(level-1)) & 0x7f;
}

void vme_enable_interrupt(void){
  CAENVME_IRQEnable(BHandle, irqmask);
}

void vme_disable_interrupt(void){
  CAENVME_IRQDisable(BHandle, irqmask);
}

/** tout = in milliseconds, return 0=no interrupt, 1=is intterupt */
/* CAENVME_IRQWait doesn't work as I needed */
int vme_wait_interrupt(int tout){
  CAENVME_IRQWait(BHandle, irqmask, tout);

  return vme_check_interrupt();
}

/** return 0=no interrupt, 1=is intterupt */
int vme_check_interrupt(void){
  int ret = 0, intmsk, vect;
  unsigned char msk;

  CAENVME_IRQCheck(BHandle, &msk);
  intmsk = msk & 0x007f;

  if(msk && irqmask){
    ret = 1;
    CAENVME_IACKCycle(BHandle, intmsk, &vect, cvD32);
  }

  return ret;
}

// size in character
int dma_vread32(unsigned int addr, char *buff, int size){
  int rsz;

  CAENVME_BLTReadCycle(BHandle, addr, buff, size, 0x0b, cvD32, &rsz);
  
  return rsz;
}


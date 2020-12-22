#ifndef LIBBBCAENVME_H
#define LIBBBCAENVME_H
#include <CAENVMEtypes.h>
#endif

int init_caen(CVBoardTypes VMEBoard, short Link, short Device);
int init_caen_v2718(void);
int init_caen_v1718(void);
unsigned short vme_read16(unsigned int addr);
unsigned int vme_read32(unsigned int addr);
int vme_write16(unsigned int addr,unsigned short sval);
int vme_write32(unsigned int addr,unsigned int lval);
int vme_amsr(unsigned int lval);
void release_caen();
int vread32(unsigned int addr, int *val);
short vread16(unsigned int addr, short *val);
void vwrite32(unsigned int addr, int *val);
void vwrite16(unsigned int addr, short *val);
int dma_vread32(unsigned int addr, char *buff, int size);
void vme_define_intlevel(int level);
void vme_enable_interrupt(void);
void vme_disable_interrupt(void);
/** tout = in milliseconds, return 0=no interrupt, 1=is intterupt */
int vme_wait_interrupt(int tout);
/** better to user vme_check_interrupt instead of wait interrupt*/
int vme_check_interrupt(void);

#ifndef __CAEN_LIB__
#define __CAEN_LIB__

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>   /* C99 compliant compilers: uint64_t */
#include <ctype.h>    /* toupper() */
#include <sys/time.h>
#include <iostream>

/* Register */
#define V1190_OUTBUFF         0x0000
#define V1190_SOFT_CLEAR      0x1016
#define V1190_ALMOST_FULL     0x1022
#define V1190_INTLEVEL        0x100a
#define V1190_CTRL_REG        0x1000
#define V1190_DATA_RDY        0x1002
/* Bit */
#define V1190_SOFT_CLEAR_BIT  1

#define V1190_TYPE_MASK_S          0xf800
#define V1190_GLOBAL_HEADER_BIT_S  0x4000
#define V1190_TDC_HEADER_BIT_S     0x0800
#define V1190_TDC_DATA_BIT_S       0x0000
#define V1190_TDC_TRAILER_BIT_S    0x1800
#define V1190_TDC_ERROR_BIT_S      0x2000
#define V1190_GLOBAL_TRAILER_BIT_S 0x8000

#define V1190_TYPE_MASK          0xf8000000
#define V1190_GLOBAL_HEADER_BIT  0x40000000
#define V1190_TDC_HEADER_BIT     0x08000000
#define V1190_TDC_DATA_BIT       0x00000000
#define V1190_TDC_TRAILER_BIT    0x18000000
#define V1190_TDC_ERROR_BIT      0x20000000
#define V1190_GLOBAL_TRAILER_BIT 0x80000000

/* Register */
#define V792_SOFT_CLEAR      0x1016
#define V792_OUTBUFF        0x0000   /* - 0x07ff (D32) */
#define V792_BIT_SET2       0x1032
#define V792_BIT_CLE2       0x1034
#define V792_BIT_CLEEVTCNT       0x1040

#define V792_CTRL_REG1      0x1010
#define V792_EVT_TRIG_REG   0x1020
#define V792_INT_REG1       0x100a


#define V775_SOFT_CLEAR_BIT  1
#define V775_BIT_CLEEVTCNT       0x1040
#define V775_SOFT_CLEAR      0x1016
#define V775_BIT_SET2       0x1032
#define V775_BIT_CLE2       0x1034

#define V775_CTRL_REG1      0x1010
#define V775_EVT_TRIG_REG   0x1020
#define V775_INT_REG1       0x100a


#ifdef __cplusplus
extern "C" {
#endif
#include "CAENVMEtypes.h"
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include "CAENVMElib.h"
#ifdef __cplusplus
}
#endif

int init_nbbqvio(int* ret, short Link, short Device);
unsigned short vme_read16(int BHandle,unsigned int addr);
unsigned int vme_read32(int BHandle,unsigned int addr);
int vme_readBlock32(int BHandle,unsigned int addr,unsigned int* Data,int size);
int vme_write16(int BHandle,unsigned int addr,unsigned short sval);
int vme_write16a16(int BHandle,unsigned int addr,unsigned short sval);
int vme_write16a32(int BHandle,unsigned int addr,unsigned short sval);
int vme_write32(int BHandle,unsigned int addr,unsigned int lval);
int vme_amsr(unsigned int lval);
int vread32(int BHandle,unsigned int addr, int *val);
short vread16(int BHandle,unsigned int addr, short *val);
void vwrite32(int BHandle,unsigned int addr, int *val);
void vwrite16(int BHandle,unsigned int addr, short *val);
void vwrite16a16(int BHandle,unsigned int addr, short *val);
void vwrite16a32(int BHandle,unsigned int addr, short *val);
void release_nbbqvio(int BHandle);
int dma_vread32(int BHandle,unsigned int addr, char *buff, int size);

#endif

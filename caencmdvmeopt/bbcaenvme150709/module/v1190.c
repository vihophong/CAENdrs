/* V1190 for libbabies + CAENLIB */

#include "libbabies.h"
#include "libbbcaenvme.h"
#include "v1190.h"

/* Segment data */
int v1190_segdata(unsigned int maddr){
  int wordcnt;

  wordcnt = 0;
  /* Global Header */
  vread32(maddr + V1190_OUTBUFF, (int *)(babies_segpt32()));
  mp += 2;
  segmentsize += 2;
  wordcnt ++;

  if((*((int *)(babies_pt()-4)) & V1190_TYPE_MASK) == V1190_GLOBAL_HEADER_BIT){
    while(wordcnt < 256){
      /* TDC Header */
      vread32(maddr + V1190_OUTBUFF, (int *)(babies_segpt32()));
      mp += 2;
      segmentsize += 2;
      wordcnt ++;
      if((*((int *)(babies_pt()-4)) & V1190_TYPE_MASK) == V1190_TDC_HEADER_BIT){
	while(wordcnt < 256){
	  vread32(maddr + V1190_OUTBUFF, (int *)(babies_segpt32()));
	  mp += 2;
	  segmentsize += 2;
	  wordcnt ++;
	  if((*((int *)(babies_pt()-4)) & V1190_TYPE_MASK) != V1190_TDC_DATA_BIT){
	    /* TDC Trailer or TDC Error */
	    break;
	  }
	}
      }else{
	break;
	/* Global Trailer */
      }
    }
  }

  return segmentsize;
}

// cnt = word count in 32bit data
int v1290_dmasegdata(unsigned int maddr, int cnt){
  int sz, rsz;

  sz = cnt *4;

  rsz = dma_vread32(maddr, babies_pt(), sz);
  babies_segptfx(rsz/2);

  return rsz/2;
}

/* Software clear */
void v1190_clear(unsigned int maddr){
  short sval;

  sval = V1190_SOFT_CLEAR_BIT;
  vwrite16(maddr + V1190_SOFT_CLEAR, &sval);
}

/* Set Almost full register */
void v1190_tdcfull(unsigned int maddr, short fl){

  vwrite16(maddr + V1190_ALMOST_FULL, &fl);
}

void v1190_intlevel(unsigned int maddr, short level){
  
  vwrite16(maddr + V1190_INTLEVEL, &level);
}

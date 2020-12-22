/* cmdvme
   use /dev/nbbqvio
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "CAENVMElib.h"
#include "CAENVMEtypes.h"

#define TIMEOUT 0x20000
#define OPADDR   0x102e
#define HSADDR   0x1030
#define OPWOK    0x01
#define OPROK    0x02

int BHandle;

#include "caenvmeA32.c"

int main(int argc, char *argv[]){
  unsigned long addr,lval,i,j;
  unsigned short sval,wval,ro=0,wo=0;

  if(argc < 3){
    //init_nbbqvio();
    printf("cmdvme -[wr/lr/ww/lw/or/ow/on/qr/qw/qn] addr [val]\n");
    printf("cmdvme -am val\n");
    exit(0);
  }else{
    init_nbbqvio();
    if(strcmp(argv[1],"-wr") == 0){
      addr = strtol(argv[2],NULL,0);
      sval = vme_read16(addr);
      printf("%04x\n",sval);
    }else if(strcmp(argv[1],"-lr") == 0){
      addr = strtol(argv[2],NULL,0);
      lval = vme_read32(addr);
      printf("%08lx\n",lval);
    }else if(strcmp(argv[1],"-ww") == 0){
      addr = strtol(argv[2],NULL,0);
      sval = strtol(argv[3],NULL,0);
      vme_write16(addr,sval);
    }else if(strcmp(argv[1],"-lw") == 0){
      addr = strtol(argv[2],NULL,0);
      lval = strtol(argv[3],NULL,0);
      vme_write32(addr,lval);
    }else if(strcmp(argv[1],"-am") == 0){
      lval = strtol(argv[2],NULL,0);
      vme_amsr(lval);
    }else if(strcmp(argv[1],"-qr") == 0){
      addr = strtol(argv[2],NULL,0);
      sval = strtol(argv[3],NULL,0);
      wval = strtol(argv[4],NULL,0);
      if(wval == 0 || wval > 8){
	wval = 1;
      }
      i=0;
      while(wo !=OPWOK && i<TIMEOUT){
	wo = vme_read16(addr+HSADDR);
	i++;
      }
      if(i<TIMEOUT){
	usleep(10);
	vme_write16(addr+OPADDR,sval);
	for(j=0;j<wval;j++){
	  i=0;
	  while(ro !=OPROK && i<TIMEOUT){
	    ro = vme_read16(addr+HSADDR);
	    i++;
	  }
	  if(i<TIMEOUT){
	    usleep(10);
	    sval = vme_read16(addr+OPADDR);
	    printf("%x\n",sval);
	  }else{
	    printf("Time out in read ok.\n");
	  }
	}
      }else{
	printf("Time out in write ok.\n");
      }
    }else if(strcmp(argv[1],"-qw") == 0){
      addr = strtol(argv[2],NULL,0);
      sval = strtol(argv[3],NULL,0);
      wval = strtol(argv[4],NULL,0);
      i=0;
      while(wo !=OPWOK && i<TIMEOUT){
	wo = vme_read16(addr+HSADDR);
	i++;
      }
      if(i<TIMEOUT){
	usleep(10);
	vme_write16(addr+OPADDR,sval);
	i=0;
	while(wo !=OPWOK && i<TIMEOUT){
	  wo = vme_read16(addr+HSADDR);
	  i++;
	}
	if(i<TIMEOUT){
	  usleep(10);
	  vme_write16(addr+OPADDR,wval);
	  printf("%x\n",sval);
	}else{
	  printf("Time out in write ok.\n");
	}
      }else{
	printf("Time out in write ok.\n");
      }
    }else if(strcmp(argv[1],"-qn") == 0){
      addr = strtol(argv[2],NULL,0);
      sval = strtol(argv[3],NULL,0);
      i=0;
      while(wo !=OPWOK && i<TIMEOUT){
	wo = vme_read16(addr+HSADDR);
	i++;
      }
      if(i<TIMEOUT){
	usleep(10);
	vme_write16(addr+OPADDR,sval);
      }else{
      printf("Time out in write ok.\n");
      }
    }else if(strcmp(argv[1],"-or") == 0){
      addr = strtol(argv[2],NULL,0);
      sval = strtol(argv[3],NULL,0);
      wval = strtol(argv[4],NULL,0);
      if(wval == 0 || wval > 8){
	wval = 1;
      }
      i=0;
      while(wo !=0x02 && i<TIMEOUT){
	wo = vme_read16(addr+0x50);
	i++;
      }
      if(i<TIMEOUT){
	usleep(10);
	vme_write16(addr+0x52,sval);
	for(j=0;j<wval;j++){
	  i=0;
	  while(ro !=0x01 && i<TIMEOUT){
	    ro = vme_read16(addr+0x50);
	    i++;
	  }
	  if(i<TIMEOUT){
	    usleep(10);
	    sval = vme_read16(addr+0x52);
	    printf("%x\n",sval);
	  }else{
	    printf("Time out in read ok.\n");
	  }
	}
      }else{
	printf("Time out in write ok.\n");
      }
    }else if(strcmp(argv[1],"-ow") == 0){
      addr = strtol(argv[2],NULL,0);
      sval = strtol(argv[3],NULL,0);
      wval = strtol(argv[4],NULL,0);
      i=0;
      while(wo !=0x02 && i<TIMEOUT){
	wo = vme_read16(addr+0x50);
	i++;
      }
      if(i<TIMEOUT){
	usleep(10);
	vme_write16(addr+0x52,sval);
	i=0;
	while(wo !=0x02 && i<TIMEOUT){
	  wo = vme_read16(addr+0x50);
	  i++;
	}
	if(i<TIMEOUT){
	  usleep(10);
	  vme_write16(addr+0x52,wval);
	  printf("%x\n",sval);
	}else{
	  printf("Time out in write ok.\n");
	}
      }else{
	printf("Time out in write ok.\n");
      }
    }else if(strcmp(argv[1],"-on") == 0){
      addr = strtol(argv[2],NULL,0);
      sval = strtol(argv[3],NULL,0);
      i=0;
      while(wo !=0x02 && i<TIMEOUT){
	wo = vme_read16(addr+0x50);
	i++;
      }
      if(i<TIMEOUT){
	usleep(10);
	vme_write16(addr+0x52,sval);
      }else{
      printf("Time out in write ok.\n");
    }
    }else{
      printf("cmdvme -[wr/lr/ww/lw/or/ow/qr/qw] addr [val]\n");
      printf("cmdvme -am val\n");
    }
    release_nbbqvio();
  }


  return 0;
}

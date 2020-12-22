/* 
 * example to use libbabies + CAENVMELib
 * H.B. RIKEN
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "libbbcaenvme.h"
#include "libbabies.h"
#include "bbpid.h"
#include "segidlist.h"

#include "v792.h"
#include "v1290.h"

int efn = 0;
unsigned int scrbuff[32] = {0};
char pidpath[] = "/tmp/babies";
//to use babies
//char pidpath[] = "/var/run/babies";


// address of VME board
const unsigned int qdcaddr = 0x11010000;
const unsigned int tdcaddr = 0x00000000;
const int intlevel = 1;

void quit(void){
  release_caen();
  rmpid(pidpath);
  printf("Exit\n");
}

void start(void){
  // Initialization for V1290 is in ../init/v1290.sh
  v1290_clear(tdcaddr);
  v792_clear(qdcaddr);
  v792_noberr(qdcaddr);
  v792_multievtberr(qdcaddr);
  v792_intlevel(qdcaddr, intlevel);
  vme_define_intlevel(intlevel);
  vme_enable_interrupt();
  printf("Start\n");
}


void stop(void){
  vme_disable_interrupt();
  printf("Stop\n");
}

void reload(void){
  printf("Reload\n");
}

void sca(void){
  int i;
  //printf("Sca\n");
  for(i=0;i<32;i++){
    scrbuff[i] = scrbuff[i] + 1;
  }
  babies_init_ncscaler(efn);
  babies_scrdata((char *)scrbuff, sizeof(scrbuff));
  babies_end_ncscaler32();
}

void clear(void){
  // write clear function i.e. send end-of-busy pulse
  //printf("Clear\n");
}

// thread
void evtloop(void){
  int status;

  while((status = babies_status()) != -1){
    switch(status){
    case STAT_RUN_IDLE:
      /* noop */
      usleep(1000);
      break;
    case STAT_RUN_START:
    case STAT_RUN_NSSTA:
      if(vme_check_interrupt()){
	/* continue */
	//printf("interrupt ok\n");
      }else{
	//printf("interrupt non\n");
	usleep(100);
	break;
      }

      //printf("evtloop\n");
      
      babies_init_event();
      //babies_init_segment(segid)
      //MKSEGID(device, focalplane, detector, module)
      //please see segidlist.h
      //module is important, e.g. C16 is 16bit data
      //device, focalplane, detector, can be defined as you want

      // Read data from V1290
      babies_init_segment(MKSEGID(0, 0, PLAT, V1190));
      //v1290_segdata(tdcaddr);
      v1290_dmasegdata(tdcaddr, 256);
      babies_end_segment();


      // Read data from V792
      babies_init_segment(MKSEGID(0, 0, PLAQ, V792));
      //v792_segdata(qdcaddr);
      v792_dmasegdata(qdcaddr, 34);
      babies_end_segment();

      // End of event
      babies_end_event();

      // babies_chk_block(int maxbuff)
      // if block size is larger than maxbuff,
      //  data should be send to the event builder
      //  i.e., read scaler and flush
      // example : 8000 = 16kB
      if(babies_chk_block(8000)){
	sca();
	babies_flush();
      }

      // clear module is here (not in clear())
      v1290_clear(tdcaddr);
      v792_clear(qdcaddr);

      clear();

      break;
    case STAT_RUN_WAITSTOP:
      // for the last sequense of run
      sca();
      babies_flush();
      babies_last_block();
      break;
    default:
      break;
    }
  }

  // write codes to quit safely

}

int main(int argc, char *argv[]){

  if(argc != 2){
    printf("babies EFN\n");
    exit(0);
  }else{
    efn = strtol(argv[1], NULL, 0);
  }

  babies_quit(quit);
  babies_start(start);
  babies_stop(stop);
  babies_reload(reload);
  babies_evtloop(evtloop);
  babies_name("babies");

  babies_init(efn);
  babies_check();

  /* init vme */
  init_caen_v2718();
  vme_amsr(0x09);

  //mkpid to use babimo
  //default = /var/run/babies
  //in this example, use /tmp/babies
  mkpid(pidpath);

  babies_main();

  return 0;
}

/* V1190 for libbabies + CAENLIB */

#include "libbabies.h"
#include "libbbcaenvme.h"
#include "sis3350.h"

// cnt = word count in 16bit data
int sis3350_dmasegdata(unsigned int maddr, int cnt){
  int sz, rsz;

  sz = cnt *2; // add header words

  rsz = dma_vread32(maddr, babies_pt(), sz);
  babies_segptfx(rsz/2);

  return rsz/2;
}

void sis3350_endaddress(unsigned int maddr, int thr){
  vwrite32(maddr + SIS3350_END_ADDRESS_THRESHOLD_ALL_ADC, &thr);
}

void sis3350_enable_interrupt(unsigned int maddr, int irq){
  int val;
  val = SIS3350_IRQ_CONF_ROAK_MODE | SIS3350_IRQ_CONF_VME_IRQ_ENABLE | 
    ((0x07 & irq) << 8);
  vwrite32(maddr + SIS3350_IRQ_CONFIG, &val);

}

void sis3350_disable_interrupt(unsigned int maddr){
  int val = 0;

  vwrite32(maddr + SIS3350_IRQ_CONFIG, &val);

}

void sis3350_enable_irqsource(unsigned int maddr, int source){
  int val;

  val = (0x00ff & source) << 16;
  vwrite32(maddr + SIS3350_IRQ_CONTROL, &val);     // Disable/Clear IRQ
  val = 0x80000000;
  vwrite32(maddr + SIS3350_IRQ_CONTROL, &val);     // Update IRQ Pulse
  vwrite32(maddr + SIS3350_IRQ_CONTROL, &source);  // Enable IRQ
}

void sis3350_enable_addrthirq(unsigned int maddr){
  // Enable reached Address Threshold (edge seinsitive) IRQ
  sis3350_enable_irqsource(maddr, 0x04);
}

void sis3350_acqmode_triggerstart(unsigned int maddr, int cnt){
  int val = 0x0105;
  int clr = 0xffff0000;
  // Direct Memory Trigger Start Mode
  // Enable external Lemo Trg In
  // Frequency synthesizer clock source

  vwrite32(maddr + SIS3350_ACQUISITION_CONTROL, &clr);
  vwrite32(maddr + SIS3350_ACQUISITION_CONTROL, &val);
  vwrite32(maddr + SIS3350_DIRECT_MEMORY_SAMPLE_LENGTH, &cnt);
}

void sis3350_frequency(unsigned int maddr, int freq){
  vwrite32(maddr + SIS3350_FREQUENCE_SYNTHESIZER, &freq);
}


int sis3350_config_nimtrigger(unsigned int maddr){
  int val = 0x10;
  vwrite32(maddr + SIS3350_CONTROL_STATUS, &val); // for NIM signal as a trigger
  return sis3350_write_dac_offset(maddr + SIS3350_EXT_CLOCK_TRIGGER_DAC_CONTROL_STATUS, 1, 31500);
}

void sis3350_clear_timestamp(unsigned int maddr){
  int val = 0x01;
  vwrite32(maddr + SIS3350_KEY_TIMESTAMP_CLR, &val);
}

void sis3350_reset(unsigned int maddr){
  int val = 0;
  vwrite32(maddr + SIS3350_KEY_RESET, &val);
}

void sis3350_arm_sampling(unsigned int maddr){
  int val = 0;
  vwrite32(maddr + SIS3350_KEY_ARM, &val);
}

void sis3350_arm_trigger(unsigned int maddr){
  int val = 0;
  vwrite32(maddr + SIS3350_KEY_TRIGGER, &val);
}

  

/* --------------------------------------------------------------------------
   SIS3350 DAC Offsets 
   module_addr			    vme module base address
   offset Value_array		DAC offset value (16 bit)
-------------------------------------------------------------------------- */
int sis3350_write_dac_offset(unsigned int dac_addr, unsigned int dac_no, unsigned int dac_value ){
  unsigned int data, addr;
  unsigned int max_timeout, timeout_cnt;

  data =  dac_value ;
  addr = dac_addr + 4 ; // DAC_DATA
  vwrite32(addr, (int *)&data);

  data =  1 + (dac_no << 4); // write to DAC Register
  addr = dac_addr ;
  vwrite32(addr, (int *)&data);

  max_timeout = 5000 ;
  timeout_cnt = 0 ;
  addr = dac_addr  ;
  do {
    vread32(addr, (int *)&data);
    timeout_cnt++;
  } while (((data & 0x8000) == 0x8000) && (timeout_cnt <  max_timeout));
  if (timeout_cnt >=  max_timeout) {
    return -2 ;
  }
		
  data =  2 + (dac_no << 4); // Load DACs 
  addr = dac_addr;
  vwrite32(addr, (int *)&data);

  timeout_cnt = 0;
  addr = dac_addr;
  do {
    vread32(addr, (int *)&data);
    timeout_cnt++;
  } while (((data & 0x8000) == 0x8000) && (timeout_cnt <  max_timeout));
  if (timeout_cnt >=  max_timeout){
    return -3;
  }

  return 1;
}

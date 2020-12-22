#ifndef __DAQ_DEVICE_CAENADC__
#define __DAQ_DEVICE_CAENADC__


#include <daq_device.h>
#include <stdio.h>
#include <CAENdrsTriggerHandler.h>
#include <CAENDigitizerType.h>


#define V792_SEPARATION_BIT 0x98000546


class daq_device_caenadc: public  daq_device {


public:

  daq_device_caenadc(const int eventtype
		     , const int subeventid
                     , const int linknumber = 0
                     , const int boardnumber = 0
                     , const int address = 0
                     , const int trigger = 1);
    
  ~daq_device_caenadc();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

  int init();
  int rearm( const int etype);
  int endrun();

 protected:
  void adc_connect(short linkNum, short boardNum);
  void adc_init();

  int adc_getHandle(){return adc_Handle;}
  void adc_setHandle(int bhandle){adc_Handle = bhandle;}
  int adc_getConnectStatus(){return adc_isConnected;}

  int adc_read_segdata(int *data);
  int adc_blockread_segdata(char* blkdata,int cnt);
  void adc_clear();
  void adc_clearEvtCounter();

  void adc_noberr();
  void adc_multievtberr();
  void adc_intlevelmulti(short level, short evtn);
  void adc_intlevel(short level);

  unsigned int adc_mAddr;
  int adc_Handle;
  int adc_isConnected;


  int _broken;
  int _warning;

  subevtdata_ptr sevt;

  int _linknumber;
  int _boardnumber;

  int blockCounter;
  int receivedTrigger;


  int _trigger;
  int _trigger_handler;

  uint32_t AllocatedSize, BufferSize, NumEvents;


  CAENdrsTriggerHandler *_th;

};


#endif

#ifndef __DAQ_DEVICE_CAENDRS_STD__
#define __DAQ_DEVICE_CAENDRS_STD__


#include <daq_device.h>
#include <stdio.h>
#include <CAENdrsTriggerHandler.h>
#include <CAENDigitizerType.h>


class daq_device_CAENdrs_std: public  daq_device {


public:

  daq_device_CAENdrs_std(const int eventtype
			 , const int subeventid
			 , const int linknumber = 0
			 , const int trigger = 1
			 , const int speed = 0
			 , const int delay = 0);
    
  ~daq_device_CAENdrs_std();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

  int init();
  int rearm( const int etype);
  int endrun();

 protected:

  int ClearConfigRegisterBit( const int bit);
  int SetConfigRegisterBit( const int bit);
  float getGS() const;

  int _broken;
  int _warning;

  subevtdata_ptr sevt;

  int handle;

  int _trigger;
  int _trigger_handler;
  int _linknumber;
  CAEN_DGTZ_DRS4Frequency_t _speed;
  int _delay;

  CAEN_DGTZ_X742_EVENT_t *_Event742;
  uint32_t AllocatedSize, BufferSize, NumEvents;


  CAENdrsTriggerHandler *_th;

};


#endif

#ifndef __DAQ_DEVICE_CAENTDC__
#define __DAQ_DEVICE_CAENTDC__


#include <daq_device.h>
#include <stdio.h>
#include <CAENdrsTriggerHandler.h>
#include <CAENDigitizerType.h>

#define V775_SEPARATION_BIT 0x98000545

class daq_device_caentdc: public  daq_device {


public:

    daq_device_caentdc(const int eventtype
                       , const int subeventid
                       , const int linknumber = 0
                       , const int boardnumber = 0
                       , const int address = 0
                       , const int trigger = 1);
    
    ~daq_device_caentdc();


    void identify(std::ostream& os = std::cout) const;

    int max_length(const int etype) const;

    // functions to do the work

    int put_data(const int etype, int * adr, const int length);

    int init();
    int rearm( const int etype);
    int endrun();

   protected:
    void tdc_connect(short linkNum, short boardNum);
    void tdc_init();

    int tdc_getHandle(){return tdc_Handle;}
    void tdc_setHandle(int bhandle){tdc_Handle = bhandle;}
    int tdc_getConnectStatus(){return tdc_isConnected;}

    int tdc_read_segdata(int *data);
    int tdc_blockread_segdata(char* blkdata,int cnt);
    void tdc_clear();
    void tdc_clearEvtCounter();

    void tdc_noberr();
    void tdc_multievtberr();
    void tdc_intlevelmulti(short level, short evtn);
    void tdc_intlevel(short level);


    unsigned int tdc_mAddr;
    int tdc_Handle;
    int tdc_isConnected;


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

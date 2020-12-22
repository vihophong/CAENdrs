#ifndef __DAQ_DEVICE_CAEN_V1730__
#define __DAQ_DEVICE_CAEN_V1730__


#include <daq_device.h>
#include <stdio.h>
#include <CAENdrsTriggerHandler.h>
#include <CAENDigitizerType.h>

#include <caen_dgtz_lib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_CH_1730SD 8

//unset if you dont want FIFO
//#define MAX_NEVT_FIFO_SEND 20

class daq_device_caen_v1730: public  daq_device {

public:
    daq_device_caen_v1730(const int eventtype
               , const int subeventid
               , const int isdppmode = 0
               , const int boardid = 0
               , const int trigger = 1);

    ~daq_device_caen_v1730();



    void identify(std::ostream& os = std::cout) const;

    int max_length(const int etype) const;

    // functions to do the work

    int put_data(const int etype, int * adr, const int length);

    int init();
    int rearm( const int etype);
    int endrun();

   protected:
    int _broken;
    int _warning;

    subevtdata_ptr sevt;

    int handle;

    int _trigger;
    int _trigger_handler;
    int _linknumber;
    CAEN_DGTZ_DRS4Frequency_t _speed;
    int _delay;

    uint32_t AllocatedSize, BufferSize, NumEvents;


    CAENdrsTriggerHandler *_th;


    int receivedTrigger;

    bool is_malloc;

    // For V1730 wavedump
    WaveDumpConfig_t   WDcfg;
    WaveDumpRun_t      WDrun;
    int ret;
    ERROR_CODES ErrCode;
    int Nb, Ne;
    char *buffer = NULL;
    char *EventPtr = NULL;
    char ConfigFileName[100];
    int isVMEDevice, MajorNumber;
    uint64_t CurrentTime, PrevRateTime, ElapsedTime;
    float ReadoutRate;
    float TriggerRate;
    int nCycles;    

    CAEN_DGTZ_BoardInfo_t       BoardInfo;
    CAEN_DGTZ_EventInfo_t       EventInfo;

    CAEN_DGTZ_UINT16_EVENT_t    *Event16=NULL; /* generic event struct with 16 bit data (10, 12, 14 and 16 bit digitizers */

    CAEN_DGTZ_UINT8_EVENT_t     *Event8=NULL; /* generic event struct with 8 bit data (only for 8 bit digitizers) */
    CAEN_DGTZ_X742_EVENT_t       *Event742=NULL;  /* custom event struct with 8 bit data (only for 8 bit digitizers) */
    CAEN_DGTZ_DRS4Correction_t X742Tables[MAX_X742_GROUP_SIZE];
    FILE *f_ini;

    int ReloadCfgStatus; // Init to the bigger positive number

    //! fifo
#ifdef MAX_NEVT_FIFO_SEND
    char myfifo[500];
    int fifobuf;
    int fifocal_prevEventCounter;
    void addfifo(int fifobufin);
#endif
};


#endif

#ifndef __DAQ_DEVICE_CAENDRS__
#define __DAQ_DEVICE_CAENDRS__


#include <daq_device.h>
#include <stdio.h>
#include <CAENdrsTriggerHandler.h>
#include <CAENDigitizerType.h>

#define MAX_N_BOARD      5
#define MAX_N_CHANNEL   16


//! some stuff from wavedump code
#define DEFAULT_CONFIG_FILE  "/etc/wavedump/WaveDumpConfig.txt"
#define GNUPLOT_DEFAULT_PATH ""
#define MAX_CH  64          /* max. number of channels */
#define MAX_SET 16           /* max. number of independent settings */

#define MAX_GW  1000        /* max. number of generic write commads */

#define PLOT_REFRESH_TIME 1000

#define VME_INTERRUPT_LEVEL      1
#define VME_INTERRUPT_STATUS_ID  0xAAAA
#define INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK
#define INTERRUPT_TIMEOUT        200  // ms

#define PLOT_WAVEFORMS   0
#define PLOT_FFT         1
#define PLOT_HISTOGRAM   2

#define CFGRELOAD_CORRTABLES_BIT (0)
#define CFGRELOAD_DESMODE_BIT (1)

typedef enum {
    OFF_BINARY=	0x00000001,			// Bit 0: 1 = BINARY, 0 =ASCII
    OFF_HEADER= 0x00000002,			// Bit 1: 1 = include header, 0 = just samples data
} OUTFILE_FLAGS;

typedef struct {
    CAEN_DGTZ_ConnectionType LinkType;
    int LinkNum;
    int ConetNode;
    uint32_t BaseAddress;
    int Nch;
    int Nbit;
    float Ts;
    int NumEvents;
    int RecordLength;
    int PostTrigger;
    int InterruptNumEvents;
    int TestPattern;
    int DesMode;
    CAEN_DGTZ_TriggerPolarity_t TriggerEdge;
    CAEN_DGTZ_IOLevel_t FPIOtype;
    CAEN_DGTZ_TriggerMode_t ExtTriggerMode;
    uint16_t EnableMask;
    char GnuPlotPath[1000];
    CAEN_DGTZ_TriggerMode_t ChannelTriggerMode[MAX_SET];
    uint32_t DCoffset[MAX_SET];
    int32_t  DCoffsetGrpCh[MAX_SET][MAX_SET];
    uint32_t Threshold[MAX_SET];
    uint8_t GroupTrgEnableMask[MAX_SET];
    uint32_t MaxGroupNumber;

    uint32_t FTDCoffset[MAX_SET];
    uint32_t FTThreshold[MAX_SET];
    CAEN_DGTZ_TriggerMode_t	FastTriggerMode;
    uint32_t	 FastTriggerEnabled;
    int GWn;
    uint32_t GWaddr[MAX_GW];
    uint32_t GWdata[MAX_GW];
    uint32_t GWmask[MAX_GW];
    OUTFILE_FLAGS OutFileFlags;
    int DecimationFactor;
    int useCorrections;
    int UseManualTables;
    char TablesFilenames[MAX_X742_GROUP_SIZE][1000];
    CAEN_DGTZ_DRS4Frequency_t DRS4Frequency;
} WaveDumpConfig_t;


typedef struct
{
  CAEN_DGTZ_ConnectionType LinkType;
  uint32_t VMEBaseAddress;
  int LinkNum;
  int BoardNum;
  uint32_t RecordLength;
  uint32_t ChannelMask;
  int EventAggr;
  CAEN_DGTZ_PulsePolarity_t PulsePolarity[16];
  CAEN_DGTZ_DPP_AcqMode_t AcqMode;
  CAEN_DGTZ_IOLevel_t IOlev;
  CAEN_DGTZ_TriggerMode_t ExtTriggerInputMode;
  uint32_t DCOffset[16];
  uint32_t PreTriggerSize[16];
  char saveDir[500];
  char saveName[500];
} DigitizerParams_t;


class daq_device_caen_v1730: public  daq_device {


public:

    daq_device_caen_v1730(const int eventtype
               , const int subeventid
               , const int isdppmode = 0
               , const int boardid = 0
               , const int trigger = 0);

    ~daq_device_caen_v1730();


  void identify(std::ostream& os = std::cout) const;

  int max_length(const int etype) const;

  // functions to do the work

  int put_data(const int etype, int * adr, const int length);

  int init();
  int rearm( const int etype);
  int endrun();

 protected:

  int combineint(uint16_t lsb,uint16_t msb){
    return (int)((msb<<16u)|lsb);
  }

  uint16_t seperateint(int inint,bool islsb){
    if (islsb) return (uint16_t) (inint&0xFFFF);
    else return (uint16_t) (inint>>16);
  }

  bool checkpulse(uint16_t* pulse,int nsample, int nsbl, double threshold, CAEN_DGTZ_PulsePolarity_t pulsepol);
  bool checkpositivepulse1740(uint16_t* pulse,int nsample, int threshold,int thresholdlow);

  int ClearConfigRegisterBit( const int bit);
  int SetConfigRegisterBit( const int bit);
  int WriteRegisterBitmask(int32_t handle, uint32_t address, uint32_t data, uint32_t mask);

  void ProgramCAEN1730(int b,int trigger);



  CAEN_DGTZ_EventInfo_t eventInfo;
  CAEN_DGTZ_UINT16_EVENT_t *Evt;
  char *buffer;
  char * evtptr;
  uint32_t size;
  uint32_t numEvents;


  // baseline and polarity for Zero suspresion
  int ZS_nsbl;
  CAEN_DGTZ_PulsePolarity_t ZS_pol;

  WaveDumpConfig_t WDcfg;
  void ProgramCAEN1740(char* cfg,int trigger);
  int GetMoreBoardInfo(int handle, CAEN_DGTZ_BoardInfo_t BoardInfo, WaveDumpConfig_t *WDcfg);
  void SetDefaultConfiguration(WaveDumpConfig_t *WDcfg);
  int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg);
  int Program1740Digitizer(int handle, WaveDumpConfig_t WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);

  //float getGS() const;

  int _broken;
  int ret;
  int _warning;

  subevtdata_ptr sevt;

  int handle;

  int _trigger;
  int _trigger_handler;
  int _isdppmode;
  CAEN_DGTZ_DRS4Frequency_t _speed;


  char configfile[100];
  int b;
  DigitizerParams_t dgtz_parms[MAX_N_BOARD];
  CAEN_DGTZ_DPP_PSD_Params_t dpp_parms[MAX_N_BOARD];
  CAEN_DGTZ_DPP_PSD_Params_t getDPPParams(char* filename);
  DigitizerParams_t getDigitizerParams(char* filename);
  int ProgramDigitizer(int handle, DigitizerParams_t Params, CAEN_DGTZ_DPP_PSD_Params_t DPPParams,char* filename);
  CAEN_DGTZ_BoardInfo_t           BoardInfo;
  //! trigger counter
  int trgcnt[MAX_N_BOARD][MAX_N_CHANNEL];



  uint32_t sAllocatedSize, sBufferSize,Nb;
  char *sbuffer_t;          // readout buffer, not be used in daqloop
  void *sEvents_t[MAX_N_CHANNEL];  // events buffer
  uint32_t NumEvents_t[MAX_N_CHANNEL];
  CAEN_DGTZ_DPP_PSD_Event_t ** Events_t;  // events buffer-convert C++
  CAEN_DGTZ_DPP_PSD_Waveforms_t* Waveform_t;
  void   *sWaveform_t;         // waveforms buffer


  CAEN_DGTZ_X742_EVENT_t *_Event742;
  uint32_t AllocatedSize, BufferSize, NumEvents;
  uint32_t AllocatedSizeDPP;


  CAENdrsTriggerHandler *_th;

};


#endif


#include <iostream>

#include <caen_manager.h>
#include <string.h>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#define VME_INTERRUPT_LEVEL      1
#define VME_INTERRUPT_STATUS_ID  0xAAAA
#define INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK


using namespace std;

caen_manager::caen_manager( const int linknumber)
{

  _linknumber = linknumber;

  handle = 0;
  int node = 0;

  _broken = CAEN_DGTZ_OpenDigitizer( CAEN_DGTZ_PCI_OpticalLink, _linknumber , node, 0 ,&handle);

  if ( _broken )
    {
      cout << " Error in CAEN_DGTZ_OpenDigitizer " << endl;
      return;
    }

}


int caen_manager::SetChannelDCOffset ( const unsigned int value)
{
  int ret = 0;
  unsigned int i;

  for ( i = 0; i < 32; i++)
    {
      ret |=  SetChannelDCOffset(i, value);
    }
  return ret;
}

int caen_manager::SetChannelDCOffset ( const unsigned int index, const unsigned int value)
{
  //  if ( value > 32768) return -1;
  return  CAEN_DGTZ_SetChannelDCOffset(handle,index, value);
}

int caen_manager::GetChannelDCOffset (const unsigned int channel) const
{
  unsigned int value;
  if ( channel > 31) return -1;
  int status =   CAEN_DGTZ_GetChannelDCOffset(handle,channel, &value);
  return value;
}


int caen_manager::SetMaxNumEventsBLT ( const unsigned int i)
{
  int status =  CAEN_DGTZ_SetMaxNumEventsBLT(handle, i);


  if ( i != GetMaxNumEventsBLT() ) 
    {
      return -1;
    }
  return status;
}

unsigned int caen_manager::GetMaxNumEventsBLT() const
 {
  unsigned int i;
  int status =  CAEN_DGTZ_GetMaxNumEventsBLT(handle, &i);
  if ( status ) return status;
  return i; 
}


int caen_manager::SetExtTriggerInputMode ( const unsigned int i)
{

  CAEN_DGTZ_TriggerMode_t mode;
  
  switch (i)
    {
    case 0:
      mode = CAEN_DGTZ_TRGMODE_DISABLED;
      break;

    case 1:
      mode = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      break;

    case 2:
      mode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
      break;

    case3:
      mode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
      break;

    default:
      return -1;
    }
 
  return  CAEN_DGTZ_SetExtTriggerInputMode(handle, mode);
}


int caen_manager::GetExtTriggerInputMode () const
{

  CAEN_DGTZ_TriggerMode_t mode;

  int status =  CAEN_DGTZ_GetExtTriggerInputMode(handle, &mode);
  
  switch (mode)
    {
    case CAEN_DGTZ_TRGMODE_DISABLED:
      return 0;
      break;

    case CAEN_DGTZ_TRGMODE_EXTOUT_ONLY:
      return 1;
      break;

    case CAEN_DGTZ_TRGMODE_ACQ_ONLY:
      return 2;
      break;
      
    case CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT:
      return 3;
      break;

    default:
      return -1;

    }

  return status; 
}



int caen_manager::SetTriggerPolarity ( const int pol)
{
  int i;
  for ( i - 0; i < 32; i++)
    {
      SetTriggerPolarity (i,pol);
    }
  return 0;
}

int caen_manager::SetTriggerPolarity ( const unsigned int channel, const int i)
{

  if ( channel > 31) return -1;

  CAEN_DGTZ_TriggerPolarity_t pol;

  if (i) pol = CAEN_DGTZ_TriggerOnFallingEdge;
  else pol = CAEN_DGTZ_TriggerOnRisingEdge;
  
  return  CAEN_DGTZ_SetTriggerPolarity(handle, channel, pol);
}


int caen_manager::GetTriggerPolarity (const unsigned int channel) const 
{

  if ( channel > 31) return -1;

  CAEN_DGTZ_TriggerPolarity_t pol;
  int status =   CAEN_DGTZ_GetTriggerPolarity(handle, channel, &pol);

  if ( pol == CAEN_DGTZ_TriggerOnFallingEdge) return 1;
  else return 0;
  
}

int caen_manager::SetFastTriggerDigitizing(const int i)
{
  if (i) return CAEN_DGTZ_SetFastTriggerDigitizing (handle, CAEN_DGTZ_ENABLE);
  else return CAEN_DGTZ_SetFastTriggerDigitizing (handle, CAEN_DGTZ_DISABLE);
}

int caen_manager::GetFastTriggerDigitizing() const
{
  CAEN_DGTZ_EnaDis_t s;
  int status = CAEN_DGTZ_GetFastTriggerDigitizing(handle, &s);
  if (s) return 1;
  return 0;
}


int caen_manager::SetFastTriggerMode(const int i)
{
  CAEN_DGTZ_TriggerMode_t  mode;
  if (i) mode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
  else mode = CAEN_DGTZ_TRGMODE_DISABLED;
  return  CAEN_DGTZ_SetFastTriggerMode (handle, mode);
}
int caen_manager::GetFastTriggerMode () const
{
  CAEN_DGTZ_TriggerMode_t  mode;
  int status =  CAEN_DGTZ_GetFastTriggerMode(handle, &mode);
  if ( mode == CAEN_DGTZ_TRGMODE_ACQ_ONLY) return 1;
  else return 0;
}



int caen_manager::SetOutputSignalMode ( const int i)
{

  CAEN_DGTZ_OutputSignalMode_t mode;
  
  switch (i)
    {
    case 0:
      mode = CAEN_DGTZ_TRIGGER;
      break;

    case 1:
      mode = CAEN_DGTZ_FASTTRG_ALL;
      break;

    case 2:
      mode = CAEN_DGTZ_FASTTRG_ACCEPTED;
      break;

    case 3:
      mode = CAEN_DGTZ_BUSY;
      break;

    case 4:
      mode = CAEN_DGTZ_CLK_OUT;
      break;

    case 5:
      mode = CAEN_DGTZ_RUN;
      break;

    case 6:
      mode = CAEN_DGTZ_TRGPULSE;
      break;

    case 7:
      mode = CAEN_DGTZ_OVERTHRESHOLD;
      break;

    default:
      return -1;
    }
 
  return  CAEN_DGTZ_SetOutputSignalMode(handle, mode);
}


int caen_manager::GetOutputSignalMode () const
{
  CAEN_DGTZ_OutputSignalMode_t mode;
  int status =  CAEN_DGTZ_GetOutputSignalMode(handle, &mode);
  switch (mode)
    {
    case CAEN_DGTZ_TRIGGER:
      return 0;
      break;

    case CAEN_DGTZ_FASTTRG_ALL:
      return 1;
      break;

    case CAEN_DGTZ_FASTTRG_ACCEPTED:
      return 2;
      break;
      
    case CAEN_DGTZ_BUSY:
      return 3;
      break;

    case CAEN_DGTZ_CLK_OUT:
      return 4;
      break;

    case CAEN_DGTZ_RUN:
      return 5;
      break;

    case CAEN_DGTZ_TRGPULSE:
      return 6;
      break;

    case CAEN_DGTZ_OVERTHRESHOLD:
      return 7;
      break;

    default:
      return -1;

    }
  return status; 
}

int caen_manager::SetDRS4SamplingFrequency ( const int i)
{

  CAEN_DGTZ_DRS4Frequency_t  mode;
  
  switch (i)
    {
    case 0:
      mode = CAEN_DGTZ_DRS4_5GHz;
      break;

    case 1:
      mode = CAEN_DGTZ_DRS4_2_5GHz;
      break;

    case 2:
      mode = CAEN_DGTZ_DRS4_1GHz;
      break;

    default:
      return -1;
    }
 
  return  CAEN_DGTZ_SetDRS4SamplingFrequency(handle, mode);
}


int caen_manager::GetDRS4SamplingFrequency() const
{
  CAEN_DGTZ_DRS4Frequency_t  mode;
  int status =  CAEN_DGTZ_GetDRS4SamplingFrequency(handle, &mode);
  switch (mode)
    {
    case CAEN_DGTZ_DRS4_5GHz:
      return 0;
      break;

    case CAEN_DGTZ_DRS4_2_5GHz:
      return 1;
      break;

    case CAEN_DGTZ_DRS4_1GHz:
      return 2;
      break;
      
    default:
      return -1;
    }
  return status; 
}

//   SetRecordLength
int caen_manager::SetRecordLength(const int i)
{
  return  CAEN_DGTZ_SetRecordLength (handle, i);
}
unsigned int caen_manager::GetRecordLength () const
{
  unsigned int i;
  int status =  CAEN_DGTZ_GetRecordLength(handle, &i);
  return i;
}


//       Set / Get PostTriggerSize
int caen_manager::SetPostTriggerSize(const unsigned int i)
{
  if ( i > 100) return -1;  // this is in %
  return CAEN_DGTZ_SetPostTriggerSize (handle, i);
}
int caen_manager::GetPostTriggerSize () const
{
  unsigned int i;
  int status =  CAEN_DGTZ_GetPostTriggerSize(handle, &i);
  if (status) return status;
  return i;
}

int caen_manager::SetIOLevel(const int i)
{
  if (i)   return CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_TTL);
  return CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_NIM);
}
int caen_manager::GetIOLevel() const
{
  CAEN_DGTZ_IOLevel_t level;
  int status = CAEN_DGTZ_GetIOLevel(handle, &level);
  if ( level == CAEN_DGTZ_IOLevel_TTL) return 1;
  else return 0;
}


int caen_manager::SetAcquisitionMode(const int i)
{
  if (i)   return CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_S_IN_CONTROLLED);
  return CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
}
int caen_manager::GetAcquisitionMode() const
{
  CAEN_DGTZ_AcqMode_t mode;
  int status = CAEN_DGTZ_GetAcquisitionMode(handle, &mode);
  if ( mode == CAEN_DGTZ_S_IN_CONTROLLED) return 1;
  else return 0;
}


int caen_manager::SetGroupEnableMask(const int i)
{
  if ( i & 0xffffff00) return -1;  // only 8 bits allowed
  return CAEN_DGTZ_SetGroupEnableMask(handle, i);
}
unsigned int caen_manager::GetGroupEnableMask() const
{
  unsigned int mask;
  int status = CAEN_DGTZ_GetGroupEnableMask(handle, &mask);
  return mask;
}

int caen_manager::LoadDRS4CorrectionData(const int i)
{
  CAEN_DGTZ_DRS4Frequency_t  mode;
  
  switch (i)
    {
    case 0:
      mode = CAEN_DGTZ_DRS4_5GHz;
      break;

    case 1:
      mode = CAEN_DGTZ_DRS4_2_5GHz;
      break;

    case 2:
      mode = CAEN_DGTZ_DRS4_1GHz;
      break;

    default:
      return -1;
    }
 
  return  CAEN_DGTZ_LoadDRS4CorrectionData(handle, mode);

}

int caen_manager::EnableDRS4Correction(const int i)
{
  if (i) return CAEN_DGTZ_EnableDRS4Correction(handle);
  return CAEN_DGTZ_DisableDRS4Correction(handle);
}


int caen_manager::WriteRegister (const unsigned int adr, const unsigned int value)
{
  return CAEN_DGTZ_WriteRegister(handle, adr, value);
}
int caen_manager::ReadRegister (const unsigned int adr)
{
  unsigned int value;
  int status = CAEN_DGTZ_ReadRegister(handle, adr, &value);
  if (status ) return status;
  return value;
}


caen_manager::~caen_manager()
{
  CAEN_DGTZ_CloseDigitizer(handle);
}


int  caen_manager::init()
{
  
  if ( _broken ) 
    {     
      cout << __FILE__ << " " << __LINE__ << " error..."  << endl;
      return 0; //  we had a catastrophic failure
    } 

 
  // set one-event readout
  //  _broken |= CAEN_DGTZ_SetMaxNumEventsBLT(handle, 1);
  SetMaxNumEventsBLT(1);


  // enable adding the trigger channels to the readout
  //  _broken |= CAEN_DGTZ_SetFastTriggerDigitizing(handle,CAEN_DGTZ_ENABLE);
  SetFastTriggerDigitizing(1);

  // no trigger front panel output
  //  _broken |= CAEN_DGTZ_SetFastTriggerMode(handle,CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  SetFastTriggerMode(0);

  // set nim levels for triggers
  //  _broken |= CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_NIM);
  SetIOLevel(0);

  // extra delay after stop
  SetPostTriggerSize(0);

  //  _broken |= CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
  SetFastTriggerMode(0);

  //  _broken |= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  SetExtTriggerInputMode(2);

  // enable all 4 groups
  SetGroupEnableMask(0xf);
  SetRecordLength(1024);

  // set frequency and correction
  SetDRS4SamplingFrequency(2) ;
  LoadDRS4CorrectionData(2);
  EnableDRS4Correction(1);

  // set DC offsets

  SetChannelDCOffset(0x8f00);


  SetFastTriggerMode(1);

  return _broken;

}

void caen_manager::identify(std::ostream& os) const
{

  CAEN_DGTZ_BoardInfo_t       BoardInfo;


  if ( _broken) 
    {
      os << "CAEN 1742 Digitizer  ** not functional ** " << endl;
    }
  else
    {
      
      int ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
      if ( ret )
	{
	  cout << " Error in CAEN_DGTZ_GetInfo" << endl;
	  return;
	}
      
      os << "CAEN Digitizer " << BoardInfo.ModelName << endl;
      os << " Firmware      "     << BoardInfo.ROC_FirmwareRel << " / " << BoardInfo.AMC_FirmwareRel << endl; 
      os << " Serial Number " << BoardInfo.SerialNumber << endl;

    }
}


int caen_manager::SetConfigRegisterBit( const int bit)
{
  const unsigned int forbidden_mask = 0x0FFFE7B7;  // none of these bits must be touched
  unsigned int pattern = 1<<bit;
  if ( pattern & forbidden_mask)
    {
      cout << " attemt to set reserved bit: " << bit << endl;
      return 1;
    }

  return CAEN_DGTZ_WriteRegister(handle,CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, pattern);
}

int caen_manager::ClearConfigRegisterBit( const int bit)
{
  const unsigned int forbidden_mask = 0x0FFFE7B7;  // none of these bits must be touched
  unsigned int pattern = 1<<bit;
  if ( pattern & forbidden_mask)
    {
      cout << " attemt to set reserved bit: " << bit << endl;
      return 1;
    }

  return CAEN_DGTZ_WriteRegister(handle,CAEN_DGTZ_BROAD_CH_CLEAR_CTRL_ADD, pattern);
}


float caen_manager::getGS() const
{
  if ( _broken ) 
    {
      //      cout << __LINE__ << "  " << __FILE__ << " broken ";
      //      identify();
      return 0; //  we had a catastrophic failure
    }

  switch (GetDRS4SamplingFrequency())
    {

    case CAEN_DGTZ_DRS4_1GHz:
      return 1;
      break;

    case CAEN_DGTZ_DRS4_2_5GHz:
      return 2.5;
      break;

    case CAEN_DGTZ_DRS4_5GHz:
      return 5;
      break;

    default:
      return 0;
      break;
    }

}

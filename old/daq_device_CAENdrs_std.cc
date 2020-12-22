
#include <iostream>

#include <daq_device_CAENdrs_std.h>
#include <string.h>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#define VME_INTERRUPT_LEVEL      1
#define VME_INTERRUPT_STATUS_ID  0xAAAA
#define INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK


using namespace std;

daq_device_CAENdrs_std::daq_device_CAENdrs_std(const int eventtype
					       , const int subeventid
					       , const int linknumber
					       , const int trigger  // do I give the system trigger?
					       , const int speed
					       , const int delay)
{

  m_eventType  = eventtype;
  m_subeventid = subeventid;

  _linknumber = linknumber;

  handle = 0;
  _Event742 = 0;

  int node = 0;

  _warning = 0;

  cout << "*************** opening Digitizer" << endl;
  _broken = CAEN_DGTZ_OpenDigitizer( CAEN_DGTZ_PCI_OpticalLink, _linknumber , node, 0 ,&handle);
  cout << "*************** " << _broken  << endl;



  _broken =  CAEN_DGTZ_Reset(handle);

  if ( _broken )
    {
      cout << " Error in CAEN_DGTZ_OpenDigitizer " << _broken << endl;
      return;
    }

  _trigger_handler=0;
  if (trigger)   _trigger_handler=1;
 
  switch (speed)
    {
    case 2:
      _speed = CAEN_DGTZ_DRS4_1GHz;
      break;
      
    case 1:
      _speed = CAEN_DGTZ_DRS4_2_5GHz;
      break;
      
    case 0:
      _speed = CAEN_DGTZ_DRS4_5GHz;
      break;
      
    default:
      _speed = CAEN_DGTZ_DRS4_1GHz;
      
    }
  
  _delay = delay;

  if ( _trigger_handler )
    {
      cout << __LINE__ << "  " << __FILE__ << " registering triggerhandler " << endl;
      _th  = new CAENdrsTriggerHandler (handle, m_eventType);
      registerTriggerHandler ( _th);
    }
  else
    {
      _th = 0;
    }

}

daq_device_CAENdrs_std::~daq_device_CAENdrs_std()
{

  if (_th)
    {
      clearTriggerHandler();
      delete _th;
      _th = 0;
    }

  if ( _Event742) 
    {
      CAEN_DGTZ_FreeEvent(handle, (void**)&_Event742);
    }
  CAEN_DGTZ_CloseDigitizer(handle);

}


int  daq_device_CAENdrs_std::init()
{
  
  if ( _broken ) 
    {
      
      return 0; //  we had a catastrophic failure
    } 


  // set one-event readout
  _broken = CAEN_DGTZ_SetMaxNumEventsBLT(handle, 1);

  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }



  // enable adding the trigger channels to teh readout
  _broken = CAEN_DGTZ_SetFastTriggerDigitizing(handle,CAEN_DGTZ_ENABLE);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }


  // no trigger front panel output
  _broken = CAEN_DGTZ_SetFastTriggerMode(handle,CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }


  // set nim levels for triggers
  _broken = CAEN_DGTZ_SetIOLevel(handle, CAEN_DGTZ_IOLevel_NIM);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return  0;
    }


  if ( _delay < 0 ) 
    {
      cout << __FILE__ << " " <<  __LINE__ << " warning: delay parameter out of range, allowed is 0-100 : " << _delay  << endl;
      _delay = 0;
      _warning = 1;
    }
  if ( _delay >100  ) 
    {
      cout << __FILE__ << " " <<  __LINE__ << " warning: delay parameter out of range, allowed is 0-100 : " << _delay  << endl;
      _delay = 100;
      _warning = 1;
    }
      

  // extra delay after stop
  _broken = CAEN_DGTZ_SetPostTriggerSize(handle,_delay);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }


  // configure interrupts and trigger
  _broken |= CAEN_DGTZ_SetInterruptConfig( handle, CAEN_DGTZ_ENABLE,
                                           VME_INTERRUPT_LEVEL, VME_INTERRUPT_STATUS_ID,
                                           1, INTERRUPT_MODE);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }

  _broken = CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }

  _broken = CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }


  // enable all 4 groups
  _broken = CAEN_DGTZ_SetGroupEnableMask(handle, 0xf);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }


  // set frequency and correction
  _broken = CAEN_DGTZ_SetDRS4SamplingFrequency(handle, (CAEN_DGTZ_DRS4Frequency_t) _speed) ;
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }

  _broken = CAEN_DGTZ_LoadDRS4CorrectionData(handle, (CAEN_DGTZ_DRS4Frequency_t) _speed);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }

  _broken = CAEN_DGTZ_EnableDRS4Correction(handle);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }


  // set DC offsets
  unsigned int i;
  for ( i = 0; i < 32; i++)
    {
      _broken = CAEN_DGTZ_SetChannelDCOffset(handle,i, 0x8f00);
      if ( _broken )
	{
	  cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
	  return 0;
	}

    }

  if ( _Event742 )
    {
      _broken =  CAEN_DGTZ_AllocateEvent(handle, (void**)&_Event742);
      if ( _broken )
	{
	  cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
	  return 0;
	}

    }

  // and we trigger rearm with our event type so it takes effect
  rearm (m_eventType);

  _broken = CAEN_DGTZ_SWStartAcquisition(handle);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      return 0;
    }

  return 0;

}

// the put_data function

int daq_device_CAENdrs_std::put_data(const int etype, int * adr, const int length )
{

  if ( _broken ) 
    {
      //      cout << __LINE__ << "  " << __FILE__ << " broken ";
      //      identify();
      return 0; //  we had a catastrophic failure
    }

  if (etype != m_eventType )  // not our id
    {
      return 0;
    }

  if ( length < max_length(etype) ) 
    {
      //      cout << __LINE__ << "  " << __FILE__ << " length " << length <<endl;
      return 0;
    }


  int len = 0;

  sevt =  (subevtdata_ptr) adr;
  // set the initial subevent length
  sevt->sub_length =  SEVTHEADERLENGTH;

  // update id's etc
  sevt->sub_id =  m_subeventid;
  sevt->sub_type=4;
  sevt->sub_decoding = 85;
  sevt->reserved[0] = 0;
  sevt->reserved[1] = 0;

  uint32_t buffersize = 0;

  while ( buffersize == 0) 
    {
      // Read data from the board 
      _broken = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, (char *) &sevt->data, &buffersize);
      if ( _broken )
	{
	  cout << __FILE__ << " " << __LINE__ << " _broken = " << _broken<< endl;
	  return 1;
	}
      //      cout << __FILE__ << " " << __LINE__ << " buffersize = " << buffersize << endl;
    }

  len = buffersize /4;
  
  sevt->sub_padding = len%2;
  len += sevt->sub_padding;
  sevt->sub_length += len;
  //cout << __LINE__ << "  " << __FILE__ << " returning "  << sevt->sub_length << endl;

  return  sevt->sub_length;
}


int daq_device_CAENdrs_std::endrun()
{
  if ( _broken ) 
    {
      return 0; //  we had a catastrophic failure
    }

  //  cout << __LINE__ << "  " << __FILE__ << " ending run " << endl;
  _broken = CAEN_DGTZ_SWStopAcquisition(handle);
  if ( _broken )
    {
      cout << __FILE__ << " " <<  __LINE__ << " Error: " << _broken << endl;
      
    }
  return _broken;

}


void daq_device_CAENdrs_std::identify(std::ostream& os) const
{

  CAEN_DGTZ_BoardInfo_t       BoardInfo;


  if ( _broken) 
    {
      os << "CAEN 1742 Digitizer Event Type: " << m_eventType 
	 << " Subevent id: " << m_subeventid 
	 << " ** not functional ** " << endl;
    }
  else
    {
      
      int ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
      if ( ret )
	{
	  cout << " Error in CAEN_DGTZ_GetInfo" << endl;
	  return;
	}
      
      os << "CAEN 1742 Digitizer STD Model " << BoardInfo.ModelName
	 << " Event Type: " << m_eventType 
	 << " Subevent id: " << m_subeventid 
	 << " Firmware "     << BoardInfo.ROC_FirmwareRel << " / " << BoardInfo.AMC_FirmwareRel 
	 << " speed "  << getGS() <<  "GS"
	 << " delay "  << _delay <<  "% ";
      if (_trigger_handler) os << " *Trigger" ;
      if (_warning) os << " **** warning - check setup parameters ****";
      os << endl;

    }
}

int daq_device_CAENdrs_std::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  (14900);
}


// the rearm() function
int  daq_device_CAENdrs_std::rearm(const int etype)
{
  if ( _broken ) 
    {
      //      cout << __LINE__ << "  " << __FILE__ << " broken ";
      //      identify();
      return 0; //  we had a catastrophic failure
    }

  if (etype != m_eventType) return 0;

  return 0;
}

int daq_device_CAENdrs_std::SetConfigRegisterBit( const int bit)
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

int daq_device_CAENdrs_std::ClearConfigRegisterBit( const int bit)
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


float daq_device_CAENdrs_std::getGS() const
{
  if ( _broken ) 
    {
      //      cout << __LINE__ << "  " << __FILE__ << " broken ";
      //      identify();
      return 0; //  we had a catastrophic failure
    }

  switch ( _speed)
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


#include <iostream>

#include <daq_device_CAENdrs.h>
#include <string.h>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#define VME_INTERRUPT_LEVEL      1
#define VME_INTERRUPT_STATUS_ID  0xAAAA
#define INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK


using namespace std;

daq_device_CAENdrs::daq_device_CAENdrs(const int eventtype
				       , const int subeventid
                                       , const int linknumber
                                       , const int trigger)
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

daq_device_CAENdrs::~daq_device_CAENdrs()
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


int  daq_device_CAENdrs::init()
{
  
  if ( _broken ) 
    {
      
      return 0; //  we had a catastrophic failure
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

int daq_device_CAENdrs::put_data(const int etype, int * adr, const int length )
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


int daq_device_CAENdrs::endrun()
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


void daq_device_CAENdrs::identify(std::ostream& os) const
{

  CAEN_DGTZ_BoardInfo_t       BoardInfo;


  if ( _broken) 
    {
      os << "CAEN Digitizer Event Type: " << m_eventType 
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
      
      os << "CAEN Digitizer Model " << BoardInfo.ModelName
	 << " Event Type: " << m_eventType 
	 << " Subevent id: " << m_subeventid 
	 << " Firmware "     << BoardInfo.ROC_FirmwareRel << " / " << BoardInfo.AMC_FirmwareRel 
	 << " speed "  << getGS() <<  "GS"
	 << " delay "  << getDelay() <<  "% ";
      if (_trigger_handler) os << " *Trigger" ;
      if (_warning) os << " **** warning - check setup parameters ****";
      os << endl;

    }
}


int daq_device_CAENdrs::getDelay() const
{
  unsigned int i;
  int status =  CAEN_DGTZ_GetPostTriggerSize(handle, &i);
  return i;
}

int daq_device_CAENdrs::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  (14900);
}


// the rearm() function
int  daq_device_CAENdrs::rearm(const int etype)
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

int daq_device_CAENdrs::SetConfigRegisterBit( const int bit)
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

int daq_device_CAENdrs::ClearConfigRegisterBit( const int bit)
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


float daq_device_CAENdrs::getGS() const
{
  if ( _broken ) 
    {
      //      cout << __LINE__ << "  " << __FILE__ << " broken ";
      //      identify();
      return 0; //  we had a catastrophic failure
    }


  CAEN_DGTZ_DRS4Frequency_t  mode;
  int status =  CAEN_DGTZ_GetDRS4SamplingFrequency(handle, &mode);
  switch (mode)
    {
    case CAEN_DGTZ_DRS4_5GHz:
      return 5;
      break;

    case CAEN_DGTZ_DRS4_2_5GHz:
      return 2.5;
      break;

    case CAEN_DGTZ_DRS4_1GHz:
      return 1;
      break;
      
    default:
      return -1;
    }
  return -1; 
}


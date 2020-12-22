
#include <iostream>

#include <caen_lib.h>

#include <daq_device_caenadc.h>
#include <string.h>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#define VME_INTERRUPT_LEVEL      0
#define VME_INTERRUPT_STATUS_ID  0xAAAA
#define INTERRUPT_MODE           CAEN_DGTZ_IRQ_MODE_ROAK

#define MAX_MBLT_SIZE            (256*1024)

using namespace std;

daq_device_caenadc::daq_device_caenadc(const int eventtype
                                       , const int subeventid
                                       , const int linknumber
                                       , const int boardnumber
                                       , const int address
                                       , const int trigger)
{
  m_eventType  = eventtype;
  m_subeventid = subeventid;

  _linknumber = linknumber;
  _boardnumber = boardnumber;
  adc_mAddr = address<<16;

  adc_Handle = 0;
  _warning = 0;
  _broken = 0;

  //! Initiate the VME bridge and perform ADC reset
  cout << "*************** opening VME Bridge" << endl;
  adc_connect((short)_linknumber,(short)_boardnumber);
  if ( adc_isConnected )
    {
      cout << "Connected to VME Bridge! Reseting ADC, status:" << adc_isConnected << endl;
      cout << "ADC based address = 0x" <<std::hex<<(adc_mAddr>>16)<<std::dec<<endl;
    }else{
      cout<<"\n\n Error opening the device\n"<<endl;
      exit(0);
  }
  // ADC soft reset
  adc_init();

  //! no effect for now, reserved for future developements
  //! trigger handler
  _trigger_handler=0;
  if (trigger)   _trigger_handler=1;

  if ( _trigger_handler )
    {
      cout << __LINE__ << "  " << __FILE__ << " registering triggerhandler " << endl;
      _th  = new CAENdrsTriggerHandler(adc_Handle, m_eventType);
      registerTriggerHandler ( _th);
    }
  else
    {
      _th = 0;
    }
}

daq_device_caenadc::~daq_device_caenadc()
{

  if (_th)
    {
      clearTriggerHandler();
      delete _th;
      _th = 0;
    }

}


//!--------------------------adc lib----------------------------

void daq_device_caenadc::adc_connect(short linkNum, short boardNum){
    int ret=0;
    adc_Handle=init_nbbqvio(&ret,linkNum,boardNum);
    adc_isConnected=ret;
}
void daq_device_caenadc::adc_init()
{
  // reset the board
  short sval;
  sval = 0x0;
  vwrite16(adc_Handle,adc_mAddr + V792_SOFT_CLEAR, &sval);
  //system("./v792.sh");//external command here
}

int daq_device_caenadc::adc_blockread_segdata(char* blkdata,int cnt)
{
    int sz, rsz;
    sz = cnt *4;
    rsz = dma_vread32(adc_Handle,adc_mAddr,blkdata, sz);
    return rsz;//return size
}
int daq_device_caenadc::adc_read_segdata(int *data)
{
    vread32(adc_Handle,adc_mAddr + V792_OUTBUFF, data);
    return 1;
}
void daq_device_caenadc::adc_clear(){
  short sval;
  sval = 0x04;

  vwrite16(adc_Handle,adc_mAddr + V792_BIT_SET2, &sval);
  vwrite16(adc_Handle,adc_mAddr + V792_BIT_CLE2, &sval);
}
void daq_device_caenadc::adc_clearEvtCounter(){
  short sval;
  sval = 0x00;

  vwrite16(adc_Handle,adc_mAddr + V792_BIT_CLEEVTCNT, &sval);
}
void daq_device_caenadc::adc_noberr(){
  short sval = 0x00;

  vwrite16(adc_Handle,adc_mAddr + V792_CTRL_REG1, &sval);
}

void daq_device_caenadc::adc_multievtberr(){
  short sval = 0x20;

  vwrite16(adc_Handle,adc_mAddr + V792_CTRL_REG1, &sval);
}
void daq_device_caenadc::adc_intlevelmulti(short level, short evtn){
    vwrite16(adc_Handle,adc_mAddr + V792_EVT_TRIG_REG, &evtn);
    vwrite16(adc_Handle,adc_mAddr + V792_INT_REG1, &level);
}
void daq_device_caenadc::adc_intlevel(short level){
    this->adc_intlevelmulti(level, 1);
}
//!--------------------------adc lib----------------------------


int  daq_device_caenadc::init()
{
  
  if ( _broken ) 
    {
      return 0; //  we had a catastrophic failure
    } 
  receivedTrigger = 0;
  blockCounter = 0;
  adc_clearEvtCounter();
  adc_clear();
  adc_noberr();
  adc_multievtberr();
  adc_intlevel(VME_INTERRUPT_LEVEL);

  rearm (m_eventType);
  return 0;

}

// the put_data function

int daq_device_caenadc::put_data(const int etype, int * adr, const int length )
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

  //! tdc reading from here
  int  *d;
  int ipos=0;
  d=(int*) &sevt->data;

  //! header
  *d++ = blockCounter;//previous recorded data block
  ipos++;
  *d++ = receivedTrigger;//previous recorded trigger
  ipos++;
  *d++ = 0;//(int)lupo->readTimeStampMSB32();
  ipos++;
  *d++ = 0;//(int)lupo->readTimeStampLSB32();
  ipos++;

  //! read qdc
  uint32_t qdcbuffersize=adc_blockread_segdata((char*)(d++),MAX_MBLT_SIZE);
  buffersize+=qdcbuffersize;
  d+=qdcbuffersize/4;

  //! end of qdc bit
  *d++ = V792_SEPARATION_BIT;
  ipos+=2;
  receivedTrigger+=qdcbuffersize/72;// total number of events
  *d++ = receivedTrigger;
  ipos++;

  if (qdcbuffersize>0) blockCounter++;
  //! calculations of event lenght
  len = buffersize /4;
  sevt->sub_padding = ipos+len;
  len += len%2;
  // this is to discard empty event
  if (qdcbuffersize>0){
      sevt->sub_length += ipos+len;
      blockCounter++;
  }else{
      sevt->sub_length += 0;
  }
  //cout << __LINE__ << "  " << __FILE__ << " returning "  << sevt->sub_length << endl;
  return  sevt->sub_length;
}


int daq_device_caenadc::endrun()
{
  if ( _broken ) 
    {
      return 0; //  we had a catastrophic failure
    }
  //  cout << __LINE__ << "  " << __FILE__ << " ending run " << endl;
}


void daq_device_caenadc::identify(std::ostream& os) const
{

}


int daq_device_caenadc::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  (14900);
}


// the rearm() function
int  daq_device_caenadc::rearm(const int etype)
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



